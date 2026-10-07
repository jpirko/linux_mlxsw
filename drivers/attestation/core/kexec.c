// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
#include <linux/byteorder/generic.h>
#include <linux/device.h>
#include <linux/kexec_handover.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/sizes.h>
#include <linux/slab.h>

#include "internal.h"

#define ATTESTATION_KEXEC_MAGIC 0x41545453
#define ATTESTATION_KEXEC_VERSION 1
#define ATTESTATION_KEXEC_SIZE SZ_4M
#define ATTESTATION_KEXEC_NAME "attestation"
#define ATTESTATION_KEXEC_HEADER_SIZE 20
#define ATTESTATION_KEXEC_INSTANCE_HEADER_SIZE 32

#define ATTESTATION_KEXEC_PROP_READ BIT(0)
#define ATTESTATION_KEXEC_PROP_EXTEND BIT(1)
#define ATTESTATION_KEXEC_PROP_INDEX BIT(2)
#define ATTESTATION_KEXEC_PROP_REMOVABLE BIT(3)

struct attestation_kexec_writer {
	u8 *data;
	size_t length;
	size_t capacity;
};

struct attestation_kexec_reader {
	const u8 *data;
	size_t length;
	size_t offset;
};

static DEFINE_MUTEX(attestation_kexec_lock);
static void *attestation_kexec_out;
static const void *attestation_kexec_in;
static size_t attestation_kexec_in_size;
static bool attestation_kexec_capture_active;
static bool attestation_kexec_capture_invalid;
static bool attestation_kexec_capture_complete;

static int attestation_kexec_put(struct attestation_kexec_writer *writer,
				 const void *data, size_t length)
{
	if (length > writer->capacity - writer->length)
		return -E2BIG;
	memcpy(writer->data + writer->length, data, length);
	writer->length += length;
	return 0;
}

static int attestation_kexec_put_u32(struct attestation_kexec_writer *writer,
				     u32 value)
{
	__le32 encoded = cpu_to_le32(value);

	return attestation_kexec_put(writer, &encoded, sizeof(encoded));
}

static void attestation_kexec_patch_u32(struct attestation_kexec_writer *writer,
					size_t offset, u32 value)
{
	__le32 encoded = cpu_to_le32(value);

	if (!WARN_ON_ONCE(offset > writer->length - sizeof(encoded)))
		memcpy(writer->data + offset, &encoded, sizeof(encoded));
}

static int attestation_kexec_get(struct attestation_kexec_reader *reader,
				 void *data, size_t length)
{
	if (length > reader->length - reader->offset)
		return -EINVAL;
	memcpy(data, reader->data + reader->offset, length);
	reader->offset += length;
	return 0;
}

static int attestation_kexec_get_u32(struct attestation_kexec_reader *reader,
				     u32 *value)
{
	__le32 encoded;
	int ret;

	ret = attestation_kexec_get(reader, &encoded, sizeof(encoded));
	if (!ret)
		*value = le32_to_cpu(encoded);
	return ret;
}

static int
attestation_kexec_props_write(struct attestation_kexec_writer *writer,
			      const struct attestation_mr_props *props,
			      bool removable)
{
	u32 flags = 0;
	size_t index;
	int ret;

	if (props->readable)
		flags |= ATTESTATION_KEXEC_PROP_READ;
	if (props->extendable)
		flags |= ATTESTATION_KEXEC_PROP_EXTEND;
	if (props->has_register_index)
		flags |= ATTESTATION_KEXEC_PROP_INDEX;
	if (removable)
		flags |= ATTESTATION_KEXEC_PROP_REMOVABLE;
	ret = attestation_kexec_put_u32(writer, props->id);
	ret |= attestation_kexec_put_u32(writer, strlen(props->name));
	ret |= attestation_kexec_put_u32(writer, props->algorithms_count);
	ret |= attestation_kexec_put_u32(writer, flags);
	ret |= attestation_kexec_put_u32(writer, props->register_index);
	ret |= attestation_kexec_put(writer, props->name,
				     strlen(props->name));
	for (index = 0; !ret && index < props->algorithms_count; index++)
		ret = attestation_kexec_put_u32(
			writer, props->algorithms[index]);
	return ret;
}

static int
attestation_kexec_values_write(struct attestation_kexec_writer *writer,
			       const struct attestation_digest_set *values)
{
	size_t index;
	int ret;

	ret = attestation_kexec_put_u32(writer, values->values_count);
	for (index = 0; !ret && index < values->values_count; index++) {
		const struct attestation_digest *value = &values->values[index];
		u32 length = hash_digest_size[value->algorithm];

		ret = attestation_kexec_put_u32(writer, value->algorithm);
		ret |= attestation_kexec_put_u32(writer, length);
		ret |= attestation_kexec_put(writer, value->data, length);
	}
	return ret;
}

static int
attestation_kexec_entry_write(struct attestation_kexec_writer *writer,
			      const struct attestation_mr_log_entry *entry)
{
	const struct attestation_mr_log_entry_extend *extend;
	int ret;

	ret = attestation_kexec_put_u32(writer, entry->type);
	ret |= attestation_kexec_put_u32(writer, entry->mr_generation);
	ret |= attestation_kexec_put_u32(writer, entry->device_generation);
	ret |= attestation_kexec_put_u32(writer, entry->mr_id);
	if (ret)
		return ret;
	switch (entry->type) {
	case ATTESTATION_MR_LOG_ENTRY_INFO: {
		const struct attestation_mr_log_entry_info *info =
			container_of(entry, typeof(*info), entry);

		return attestation_kexec_props_write(writer, &info->props,
						     false);
	}
	case ATTESTATION_MR_LOG_ENTRY_EXTEND:
		extend = container_of(entry, typeof(*extend), entry);
		return attestation_kexec_values_write(writer, &extend->values);
	case ATTESTATION_MR_LOG_ENTRY_RECORD_EXTEND: {
		const struct attestation_mr_log_entry_record_extend *record =
			container_of(entry, typeof(*record), extend.entry);

		ret = attestation_kexec_values_write(
			writer, &record->extend.values);
		ret |= attestation_kexec_put_u32(
			writer, strlen(record->record_format));
		ret |= attestation_kexec_put_u32(
			writer, record->record_data_length);
		ret |= attestation_kexec_put(
			writer, record->record_format,
			strlen(record->record_format));
		ret |= attestation_kexec_put(
			writer, record->record_data,
			record->record_data_length);
		return ret;
	}
	case ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND: {
		const struct attestation_mr_log_entry_ima_extend *ima =
			container_of(entry, typeof(*ima), extend.entry);

		ret = attestation_kexec_values_write(
			writer, &ima->extend.values);
		ret |= attestation_kexec_put_u32(
			writer, strlen(ima->template_name));
		ret |= attestation_kexec_put_u32(
			writer, ima->template_data_length);
		ret |= attestation_kexec_put_u32(writer, ima->violation);
		ret |= attestation_kexec_put(
			writer, ima->template_name,
			strlen(ima->template_name));
		ret |= attestation_kexec_put(
			writer, ima->template_data,
			ima->template_data_length);
		return ret;
	}
	}
	return -EINVAL;
}

static int
attestation_kexec_mr_write(struct attestation_kexec_writer *writer,
			   struct attestation *att, struct attestation_mr *mr)
{
	struct attestation_digest_set values;
	size_t index;
	int ret;

	if (!mr->props->readable || !att->provider->mr_read)
		return -EOPNOTSUPP;
	values.values_count = mr->props->algorithms_count;
	values.values = kcalloc(values.values_count,
				sizeof(*values.values), GFP_KERNEL);
	if (!values.values)
		return -ENOMEM;
	for (index = 0; index < values.values_count; index++)
		values.values[index].algorithm = mr->props->algorithms[index];
	ret = att->provider->mr_read(att, mr->props->id, NULL, &values);
	if (!ret)
		ret = attestation_kexec_props_write(
			writer, mr->props, mr->removable);
	if (!ret)
		ret = attestation_kexec_put_u32(
			writer, mr->start_generation);
	if (!ret)
		ret = attestation_kexec_values_write(writer, &values);
	kfree(values.values);
	return ret;
}

static int
attestation_kexec_instance_write(struct attestation_kexec_writer *writer,
				 struct attestation *att)
{
	const struct ctlv_family *family =
		ctlv_device_family(att->ctlvdev);
	struct device *parent = ctlv_device_parent(att->ctlvdev);
	const char *bus = dev_bus_name(parent);
	const char *device = dev_name(parent);
	struct attestation_mr_log_entry *entry;
	struct attestation_mr *mr;
	unsigned long index;
	size_t start = writer->length;
	u32 entries = 0, mrs = 0;
	int ret;

	if (!bus || !*bus || !device || !*device || !family)
		return -EINVAL;
	xa_for_each(&att->mrs, index, mr)
		mrs++;
	xa_for_each(&att->log->entries, index, entry)
		entries++;
	ret = attestation_kexec_put_u32(writer, 0);
	ret |= attestation_kexec_put_u32(writer, 0);
	ret |= attestation_kexec_put_u32(writer, strlen(bus));
	ret |= attestation_kexec_put_u32(writer, strlen(device));
	ret |= attestation_kexec_put_u32(
		writer, strlen(family->schema->name));
	ret |= attestation_kexec_put_u32(writer, att->log->generation);
	ret |= attestation_kexec_put_u32(writer, mrs);
	ret |= attestation_kexec_put_u32(writer, entries);
	ret |= attestation_kexec_put(writer, bus, strlen(bus));
	ret |= attestation_kexec_put(writer, device, strlen(device));
	ret |= attestation_kexec_put(
		writer, family->schema->name,
		strlen(family->schema->name));
	xa_for_each(&att->mrs, index, mr) {
		if (ret)
			break;
		ret = attestation_kexec_mr_write(writer, att, mr);
	}
	xa_for_each(&att->log->entries, index, entry) {
		if (ret)
			break;
		ret = attestation_kexec_entry_write(writer, entry);
	}
	if (!ret)
		attestation_kexec_patch_u32(
			writer, start, writer->length - start);
	return ret;
}

static int attestation_kexec_capture(void)
{
	struct attestation_kexec_writer writer = {
		.data = attestation_kexec_out,
		.capacity = ATTESTATION_KEXEC_SIZE,
	};
	struct attestation *att;
	unsigned long index = 0;
	u32 instances = 0;
	int srcu_index;
	int ret;

	WRITE_ONCE(attestation_kexec_capture_complete, false);
	WRITE_ONCE(attestation_kexec_capture_invalid, false);
	WRITE_ONCE(attestation_kexec_capture_active, true);
	memset(attestation_kexec_out, 0, ATTESTATION_KEXEC_HEADER_SIZE);
	writer.length = ATTESTATION_KEXEC_HEADER_SIZE;
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	for (;;) {
		rcu_read_lock();
		att = xa_find(&attestation_instances, &index, UINT_MAX,
			      XA_PRESENT);
		rcu_read_unlock();
		if (!att)
			break;
		att = attestation_instance_lock(index);
		if (!IS_ERR(att)) {
			if (att->log) {
				att->log->frozen = true;
				ret = attestation_kexec_instance_write(
					&writer, att);
				if (ret) {
					attestation_instance_unlock(att);
					goto out_srcu;
				}
				instances++;
			}
			attestation_instance_unlock(att);
		}
		if (index == UINT_MAX)
			break;
		index++;
	}
	if (READ_ONCE(attestation_kexec_capture_invalid)) {
		ret = -ESTALE;
		goto out_srcu;
	}
	ret = 0;
	attestation_kexec_patch_u32(
		&writer, 0, ATTESTATION_KEXEC_MAGIC);
	attestation_kexec_patch_u32(
		&writer, 4, ATTESTATION_KEXEC_VERSION);
	attestation_kexec_patch_u32(&writer, 12, writer.length);
	attestation_kexec_patch_u32(&writer, 16, instances);
	/* Completeness is the final published field. */
	smp_wmb();
	attestation_kexec_patch_u32(&writer, 8, 1);
	WRITE_ONCE(attestation_kexec_capture_complete, true);
out_srcu:
	WRITE_ONCE(attestation_kexec_capture_active, false);
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	return ret;
}

static int attestation_kexec_notify(struct notifier_block *nb,
				    unsigned long action, void *data)
{
	int ret;

	if (action != SYS_RESTART || !data ||
	    strcmp(data, "kexec reboot") || !attestation_kexec_out)
		return NOTIFY_DONE;
	guard(mutex)(&attestation_kexec_lock);
	ret = attestation_kexec_capture();
	if (ret) {
		pr_err("attestation handover capture failed: %d\n", ret);
		memset(attestation_kexec_out, 0,
		       ATTESTATION_KEXEC_HEADER_SIZE);
	}
	return NOTIFY_DONE;
}

static struct notifier_block attestation_kexec_nb = {
	.notifier_call = attestation_kexec_notify,
	.priority = -1,
};

static int attestation_kexec_header_validate(const void *data, size_t size)
{
	struct attestation_kexec_reader reader = {
		.data = data,
		.length = size,
	};
	u32 magic, version, complete, length, instances;
	int ret;

	ret = attestation_kexec_get_u32(&reader, &magic);
	ret |= attestation_kexec_get_u32(&reader, &version);
	ret |= attestation_kexec_get_u32(&reader, &complete);
	ret |= attestation_kexec_get_u32(&reader, &length);
	ret |= attestation_kexec_get_u32(&reader, &instances);
	if (ret || magic != ATTESTATION_KEXEC_MAGIC ||
	    version != ATTESTATION_KEXEC_VERSION || complete != 1 ||
	    length > size || length < ATTESTATION_KEXEC_HEADER_SIZE)
		return -EINVAL;
	return 0;
}

static int
attestation_kexec_string_match(struct attestation_kexec_reader *reader,
			       u32 length, const char *string)
{
	size_t actual = strlen(string);
	int matches;

	if (length > reader->length - reader->offset)
		return -EINVAL;
	matches = length == actual &&
		  !memcmp(reader->data + reader->offset, string, length);
	reader->offset += length;
	return matches;
}

static void attestation_kexec_props_free(struct attestation_mr_props *props)
{
	kfree(props->algorithms);
	kfree(props->name);
}

static int
attestation_kexec_props_read(struct attestation_kexec_reader *reader,
			     struct attestation_mr_props *props,
			     bool *removable)
{
	enum hash_algo *algorithms;
	char *name;
	u32 id, name_len, count, flags, register_index, value;
	size_t index;
	int ret;

	ret = attestation_kexec_get_u32(reader, &id);
	ret |= attestation_kexec_get_u32(reader, &name_len);
	ret |= attestation_kexec_get_u32(reader, &count);
	ret |= attestation_kexec_get_u32(reader, &flags);
	ret |= attestation_kexec_get_u32(reader, &register_index);
	if (ret || !name_len || name_len > 128 || !count ||
	    count > HASH_ALGO__LAST)
		return -EINVAL;
	name = kmalloc(name_len + 1, GFP_KERNEL);
	algorithms = kcalloc(count, sizeof(*algorithms), GFP_KERNEL);
	if (!name || !algorithms) {
		kfree(algorithms);
		kfree(name);
		return -ENOMEM;
	}
	ret = attestation_kexec_get(reader, name, name_len);
	name[name_len] = '\0';
	for (index = 0; !ret && index < count; index++) {
		ret = attestation_kexec_get_u32(reader, &value);
		if (!ret && (value >= HASH_ALGO__LAST ||
			     !hash_digest_size[value] ||
			     (index && algorithms[index - 1] >= value)))
			ret = -EINVAL;
		algorithms[index] = value;
	}
	if (ret) {
		kfree(algorithms);
		kfree(name);
		return ret;
	}
	*props = (struct attestation_mr_props) {
		.id = id,
		.name = name,
		.algorithms = algorithms,
		.algorithms_count = count,
		.has_register_index =
			flags & ATTESTATION_KEXEC_PROP_INDEX,
		.register_index = register_index,
		.readable = flags & ATTESTATION_KEXEC_PROP_READ,
		.extendable = flags & ATTESTATION_KEXEC_PROP_EXTEND,
	};
	*removable = flags & ATTESTATION_KEXEC_PROP_REMOVABLE;
	return 0;
}

static int
attestation_kexec_values_read(struct attestation_kexec_reader *reader,
			      struct attestation_digest_set *values)
{
	u32 count, algorithm, length;
	size_t index;
	int ret;

	ret = attestation_kexec_get_u32(reader, &count);
	if (ret || !count || count > HASH_ALGO__LAST)
		return -EINVAL;
	values->values = kcalloc(count, sizeof(*values->values), GFP_KERNEL);
	if (!values->values)
		return -ENOMEM;
	values->values_count = count;
	for (index = 0; index < count; index++) {
		ret = attestation_kexec_get_u32(reader, &algorithm);
		ret |= attestation_kexec_get_u32(reader, &length);
		if (ret || algorithm >= HASH_ALGO__LAST ||
		    length != hash_digest_size[algorithm] ||
		    (index &&
		     values->values[index - 1].algorithm >= algorithm)) {
			ret = -EINVAL;
			goto err;
		}
		values->values[index].algorithm = algorithm;
		ret = attestation_kexec_get(
			reader, values->values[index].data, length);
		if (ret)
			goto err;
	}
	return 0;
err:
	kfree(values->values);
	values->values = NULL;
	values->values_count = 0;
	return ret;
}

static bool
attestation_kexec_props_equal(const struct attestation_mr_props *left,
			      const struct attestation_mr_props *right,
			      bool removable,
			      const struct attestation_mr *mr,
			      bool compare_removable)
{
	return left->id == right->id &&
	       !strcmp(left->name, right->name) &&
	       left->algorithms_count == right->algorithms_count &&
	       !memcmp(left->algorithms, right->algorithms,
		       left->algorithms_count * sizeof(*left->algorithms)) &&
	       left->has_register_index == right->has_register_index &&
	       (!left->has_register_index ||
		left->register_index == right->register_index) &&
	       left->readable == right->readable &&
	       left->extendable == right->extendable &&
	       (!compare_removable || removable == mr->removable);
}

static int
attestation_kexec_values_compare(struct attestation *att,
				 struct attestation_mr *mr,
				 const struct attestation_digest_set *saved)
{
	struct attestation_digest_set actual;
	size_t index;
	int ret;

	if (saved->values_count != mr->props->algorithms_count)
		return -EINVAL;
	actual.values_count = mr->props->algorithms_count;
	actual.values = kcalloc(actual.values_count,
				sizeof(*actual.values), GFP_KERNEL);
	if (!actual.values)
		return -ENOMEM;
	for (index = 0; index < actual.values_count; index++)
		actual.values[index].algorithm = mr->props->algorithms[index];
	ret = att->provider->mr_read(att, mr->props->id, NULL, &actual);
	if (!ret &&
	    memcmp(actual.values, saved->values,
		   actual.values_count * sizeof(*actual.values)))
		ret = -ESTALE;
	kfree(actual.values);
	return ret;
}

static int
attestation_kexec_live_mrs_restore(struct attestation *att,
				   struct attestation_kexec_reader *reader,
				   u32 count)
{
	struct attestation_digest_set values = {};
	struct attestation_mr_props props = {};
	struct attestation_mr *mr;
	unsigned long index;
	u32 start_generation;
	bool removable;
	u32 live = 0;
	int ret;

	xa_for_each(&att->mrs, index, mr)
		live++;
	if (live != count)
		return -ESTALE;
	while (count--) {
		ret = attestation_kexec_props_read(
			reader, &props, &removable);
		if (ret)
			return ret;
		ret = attestation_kexec_get_u32(reader, &start_generation);
		if (ret)
			goto out_props;
		ret = attestation_kexec_values_read(reader, &values);
		if (ret)
			goto out_props;
		mr = xa_load(&att->mrs, props.id);
		if (!mr ||
		    !attestation_kexec_props_equal(
			    &props, mr->props, removable, mr, true)) {
			ret = -ESTALE;
			goto out_values;
		}
		ret = attestation_kexec_values_compare(att, mr, &values);
		if (!ret)
			mr->start_generation = start_generation;
out_values:
		kfree(values.values);
		values = (struct attestation_digest_set) {};
out_props:
		attestation_kexec_props_free(&props);
		props = (struct attestation_mr_props) {};
		if (ret)
			return ret;
	}
	return 0;
}

static int attestation_kexec_entry_read(
	struct attestation_kexec_reader *reader,
	struct attestation_mr_log_entry **entry_out)
{
	struct attestation_mr_log_entry *entry;
	u32 type, generation, device_generation, mr_id;
	u32 name_len, data_len, violation;
	bool removable;
	int ret;

	ret = attestation_kexec_get_u32(reader, &type);
	ret |= attestation_kexec_get_u32(reader, &generation);
	ret |= attestation_kexec_get_u32(reader, &device_generation);
	ret |= attestation_kexec_get_u32(reader, &mr_id);
	if (ret || type > ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND ||
	    !generation)
		return -EINVAL;
	switch (type) {
	case ATTESTATION_MR_LOG_ENTRY_INFO: {
		struct attestation_mr_log_entry_info *info;

		info = kzalloc_obj(*info, GFP_KERNEL);
		if (!info)
			return -ENOMEM;
		ret = attestation_kexec_props_read(
			reader, &info->props, &removable);
		if (ret || removable || info->props.id != mr_id) {
			if (!ret)
				ret = -EINVAL;
			attestation_kexec_props_free(&info->props);
			kfree(info);
			return ret;
		}
		entry = &info->entry;
		break;
	}
	case ATTESTATION_MR_LOG_ENTRY_EXTEND: {
		struct attestation_mr_log_entry_extend *extend;

		extend = kzalloc_obj(*extend, GFP_KERNEL);
		if (!extend)
			return -ENOMEM;
		ret = attestation_kexec_values_read(
			reader, &extend->values);
		if (ret) {
			kfree(extend);
			return ret;
		}
		entry = &extend->entry;
		break;
	}
	case ATTESTATION_MR_LOG_ENTRY_RECORD_EXTEND: {
		struct attestation_mr_log_entry_record_extend *record;

		record = kzalloc_obj(*record, GFP_KERNEL);
		if (!record)
			return -ENOMEM;
		ret = attestation_kexec_values_read(
			reader, &record->extend.values);
		ret |= attestation_kexec_get_u32(reader, &name_len);
		ret |= attestation_kexec_get_u32(reader, &data_len);
		if (ret || !name_len || name_len > 128 || data_len > 4096) {
			ret = -EINVAL;
			goto err_record;
		}
		record->record_format = kmalloc(name_len + 1, GFP_KERNEL);
		record->record_data = kmalloc(data_len, GFP_KERNEL);
		if (!record->record_format ||
		    (data_len && !record->record_data)) {
			ret = -ENOMEM;
			goto err_record;
		}
		ret = attestation_kexec_get(
			reader, record->record_format, name_len);
		record->record_format[name_len] = '\0';
		ret |= attestation_kexec_get(
			reader, record->record_data, data_len);
		if (ret)
			goto err_record;
		record->record_data_length = data_len;
		entry = &record->extend.entry;
		break;
err_record:
		kfree(record->record_data);
		kfree(record->record_format);
		kfree(record->extend.values.values);
		kfree(record);
		return ret;
	}
	case ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND: {
		struct attestation_mr_log_entry_ima_extend *ima;

		ima = kzalloc_obj(*ima, GFP_KERNEL);
		if (!ima)
			return -ENOMEM;
		ret = attestation_kexec_values_read(
			reader, &ima->extend.values);
		ret |= attestation_kexec_get_u32(reader, &name_len);
		ret |= attestation_kexec_get_u32(reader, &data_len);
		ret |= attestation_kexec_get_u32(reader, &violation);
		if (ret || !name_len || name_len > 128 ||
		    data_len > 65536 || violation > 1) {
			ret = -EINVAL;
			goto err_ima;
		}
		ima->template_name = kmalloc(name_len + 1, GFP_KERNEL);
		ima->template_data = kmalloc(data_len, GFP_KERNEL);
		if (!ima->template_name ||
		    (data_len && !ima->template_data)) {
			ret = -ENOMEM;
			goto err_ima;
		}
		ret = attestation_kexec_get(
			reader, ima->template_name, name_len);
		ima->template_name[name_len] = '\0';
		ret |= attestation_kexec_get(
			reader, ima->template_data, data_len);
		if (ret)
			goto err_ima;
		ima->template_data_length = data_len;
		ima->violation = violation;
		entry = &ima->extend.entry;
		break;
err_ima:
		kfree(ima->template_data);
		kfree(ima->template_name);
		kfree(ima->extend.values.values);
		kfree(ima);
		return ret;
	}
	default:
		return -EINVAL;
	}
	entry->type = type;
	entry->mr_generation = generation;
	entry->device_generation = device_generation;
	entry->mr_id = mr_id;
	*entry_out = entry;
	return 0;
}

static int
attestation_kexec_log_restore(struct attestation *att,
			      struct attestation_kexec_reader *reader,
			      u32 generation, u32 count)
{
	DEFINE_XARRAY(lifetimes);
	struct attestation_mr_log_entry *entry;
	struct attestation_mr_log_entry_info *info;
	struct attestation_mr_log_entry_extend *extend;
	struct attestation_mr_log *log;
	unsigned int previous = 0;
	size_t index;
	int ret;

	log = kzalloc_obj(*log, GFP_KERNEL);
	if (!log)
		return -ENOMEM;
	xa_init(&log->entries);
	while (count--) {
		ret = attestation_kexec_entry_read(reader, &entry);
		if (ret)
			goto err_log;
		if (entry->mr_generation <= previous ||
		    entry->mr_generation > generation) {
			attestation_mr_log_entry_free(entry);
			ret = -EINVAL;
			goto err_log;
		}
		if (entry->type == ATTESTATION_MR_LOG_ENTRY_INFO) {
			info = container_of(entry, typeof(*info), entry);
			ret = xa_err(xa_store(&lifetimes, entry->mr_id,
					      info, GFP_KERNEL));
			if (ret) {
				attestation_mr_log_entry_free(entry);
				goto err_log;
			}
		} else {
			info = xa_load(&lifetimes, entry->mr_id);
			extend = container_of(entry, typeof(*extend), entry);
			if (!info ||
			    info->props.algorithms_count !=
				    extend->values.values_count) {
				attestation_mr_log_entry_free(entry);
				ret = -EINVAL;
				goto err_log;
			}
			for (index = 0;
			     index < extend->values.values_count; index++)
				if (info->props.algorithms[index] !=
				    extend->values.values[index].algorithm) {
					attestation_mr_log_entry_free(entry);
					ret = -EINVAL;
					goto err_log;
				}
		}
		ret = xa_insert(&log->entries, entry->mr_generation,
				entry, GFP_KERNEL);
		if (ret) {
			attestation_mr_log_entry_free(entry);
			goto err_log;
		}
		if (log->tail)
			rcu_assign_pointer(log->tail->next, entry);
		log->tail = entry;
		previous = entry->mr_generation;
	}
	log->generation = generation;
	att->log = log;
	xa_destroy(&lifetimes);
	return 0;
err_log:
	xa_destroy(&lifetimes);
	attestation_mr_log_free(log);
	return ret;
}

static int attestation_kexec_live_mrs_validate(struct attestation *att)
{
	struct attestation_mr_log_entry_info *info;
	struct attestation_mr_log_entry *entry;
	struct attestation_mr *mr;
	unsigned long index;

	xa_for_each(&att->mrs, index, mr) {
		entry = xa_load(&att->log->entries, mr->start_generation);
		if (!entry || entry->type != ATTESTATION_MR_LOG_ENTRY_INFO ||
		    entry->mr_id != mr->props->id)
			return -ESTALE;
		info = container_of(entry, typeof(*info), entry);
		if (!attestation_kexec_props_equal(
			    &info->props, mr->props, false, mr, false))
			return -ESTALE;
	}
	return 0;
}

static void attestation_kexec_mr_starts_clear(struct attestation *att)
{
	struct attestation_mr *mr;
	unsigned long index;

	xa_for_each(&att->mrs, index, mr)
		mr->start_generation = 0;
}

static int
attestation_kexec_instance_restore(struct attestation *att,
				   struct attestation_kexec_reader *reader,
				   u32 instance_length)
{
	struct attestation_kexec_reader instance = *reader;
	const struct ctlv_family *family =
		ctlv_device_family(att->ctlvdev);
	struct device *parent = ctlv_device_parent(att->ctlvdev);
	const char *bus = dev_bus_name(parent);
	const char *device = dev_name(parent);
	u32 consumed, bus_len, device_len, family_len;
	u32 generation, mrs, entries;
	int bus_match, device_match, family_match;
	int ret;

	instance.length = instance.offset + instance_length - sizeof(__le32);
	ret = attestation_kexec_get_u32(&instance, &consumed);
	ret |= attestation_kexec_get_u32(&instance, &bus_len);
	ret |= attestation_kexec_get_u32(&instance, &device_len);
	ret |= attestation_kexec_get_u32(&instance, &family_len);
	ret |= attestation_kexec_get_u32(&instance, &generation);
	ret |= attestation_kexec_get_u32(&instance, &mrs);
	ret |= attestation_kexec_get_u32(&instance, &entries);
	if (ret || consumed || !bus || !device || !family)
		return ret ? ret : 0;
	bus_match = attestation_kexec_string_match(
		&instance, bus_len, bus);
	device_match = attestation_kexec_string_match(
		&instance, device_len, device);
	family_match = attestation_kexec_string_match(
		&instance, family_len, family->schema->name);
	if (bus_match < 0 || device_match < 0 || family_match < 0)
		return -EINVAL;
	if (!bus_match || !device_match || !family_match)
		return 0;
	/* A matching record is single-use even when verification fails. */
	WRITE_ONCE(*((__le32 *)(reader->data + reader->offset)),
		   cpu_to_le32(1));

	ret = attestation_kexec_live_mrs_restore(att, &instance, mrs);
	if (ret)
		goto fresh_or_error;
	ret = attestation_kexec_log_restore(
		att, &instance, generation, entries);
	if (ret)
		goto fresh_or_error;
	ret = attestation_kexec_live_mrs_validate(att);
	if (ret)
		goto err_log;
	if (instance.offset != instance.length) {
		ret = -EINVAL;
		goto err_log;
	}
	return 1;

err_log:
	attestation_mr_log_free(att->log);
	att->log = NULL;
fresh_or_error:
	attestation_kexec_mr_starts_clear(att);
	return ret == -ENOMEM ? ret : 0;
}

int attestation_kexec_restore(struct attestation *att)
{
	struct attestation_kexec_reader reader;
	u32 magic, version, complete, length, instances;
	u32 instance_length;
	size_t start;
	int ret;

	guard(mutex)(&attestation_kexec_lock);
	if (!attestation_kexec_in)
		return 0;
	reader = (struct attestation_kexec_reader) {
		.data = attestation_kexec_in,
		.length = attestation_kexec_in_size,
	};
	ret = attestation_kexec_get_u32(&reader, &magic);
	ret |= attestation_kexec_get_u32(&reader, &version);
	ret |= attestation_kexec_get_u32(&reader, &complete);
	ret |= attestation_kexec_get_u32(&reader, &length);
	ret |= attestation_kexec_get_u32(&reader, &instances);
	if (ret || magic != ATTESTATION_KEXEC_MAGIC ||
	    version != ATTESTATION_KEXEC_VERSION || complete != 1 ||
	    length > reader.length ||
	    length < ATTESTATION_KEXEC_HEADER_SIZE)
		return -EINVAL;
	reader.length = length;
	while (!ret && instances--) {
		start = reader.offset;
		ret = attestation_kexec_get_u32(
			&reader, &instance_length);
		if (ret || instance_length < ATTESTATION_KEXEC_INSTANCE_HEADER_SIZE ||
		    instance_length > reader.length - start)
			return -EINVAL;
		ret = attestation_kexec_instance_restore(
			att, &reader, instance_length);
		if (ret)
			return ret;
		reader.offset = start + instance_length;
	}
	return ret ?: (reader.offset == reader.length ? 0 : -EINVAL);
}

void attestation_kexec_invalidate(struct attestation *att)
{
	if (READ_ONCE(attestation_kexec_capture_active) &&
	    !READ_ONCE(attestation_kexec_capture_complete))
		WRITE_ONCE(attestation_kexec_capture_invalid, true);
}

int attestation_kexec_init(void)
{
	phys_addr_t phys;
	size_t size;
	int ret;

	ret = register_reboot_notifier(&attestation_kexec_nb);
	if (ret)
		return ret;
	ret = kho_retrieve_subtree(ATTESTATION_KEXEC_NAME, &phys, &size);
	if (!ret && size >= ATTESTATION_KEXEC_HEADER_SIZE) {
		void *input = phys_to_virt(phys);

		attestation_kexec_in = input;
		attestation_kexec_in_size = size;
		if (attestation_kexec_header_validate(
			    attestation_kexec_in, size)) {
			kho_restore_free(input);
			attestation_kexec_in = NULL;
			attestation_kexec_in_size = 0;
		}
	}
	if (!kho_is_enabled())
		return 0;
	attestation_kexec_out = kho_alloc_preserve(ATTESTATION_KEXEC_SIZE);
	if (IS_ERR(attestation_kexec_out)) {
		ret = PTR_ERR(attestation_kexec_out);
		attestation_kexec_out = NULL;
		goto err_notifier;
	}
	ret = kho_add_subtree(ATTESTATION_KEXEC_NAME,
			      attestation_kexec_out, ATTESTATION_KEXEC_SIZE);
	if (ret)
		goto err_buffer;
	return 0;

err_buffer:
	kho_unpreserve_free(attestation_kexec_out);
	attestation_kexec_out = NULL;
err_notifier:
	unregister_reboot_notifier(&attestation_kexec_nb);
	return ret;
}

void attestation_kexec_exit(void)
{
	unregister_reboot_notifier(&attestation_kexec_nb);
	if (attestation_kexec_out) {
		kho_remove_subtree(attestation_kexec_out);
		kho_unpreserve_free(attestation_kexec_out);
		attestation_kexec_out = NULL;
	}
	if (attestation_kexec_in) {
		kho_restore_free((void *)attestation_kexec_in);
		attestation_kexec_in = NULL;
		attestation_kexec_in_size = 0;
	}
}
