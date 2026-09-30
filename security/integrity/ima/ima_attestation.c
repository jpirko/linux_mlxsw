// SPDX-License-Identifier: GPL-2.0-only
#include <linux/attestation/ima.h>
#include <linux/byteorder/generic.h>
#include <linux/hashtable.h>
#include <linux/ima.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kexec_handover.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/overflow.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/sizes.h>
#include <linux/slab.h>

#include "ima.h"

struct ima_attestation_selector {
	char *family_name;
	char *bus_name;
	char *device_name;
	unsigned int mr_id;
	int error;
	bool target_present;
	bool bus_present;
	bool device_present;
};

static struct ima_attestation_selector ima_selector;
static char ima_attestation_boot_family[256];
static char ima_attestation_boot_bus[256];
static char ima_attestation_boot_device[256];

struct ima_attestation_target *ima_attestation_target;

struct ima_attestation_digest {
	struct hlist_node node;
	struct attestation_digest values[];
};

static DEFINE_HASHTABLE(ima_attestation_digests, IMA_HASH_BITS);

#define IMA_ATTESTATION_BATCH_ENTRIES	128
#define IMA_ATTESTATION_TEMPLATE_MAX	65536
#define IMA_ATTESTATION_RECORD_DATA_MAX	4096
#define IMA_ATTESTATION_RECORD_FORMAT_MAX	128
#define IMA_ATTESTATION_KEXEC_MAGIC	0x494d4141
#define IMA_ATTESTATION_KEXEC_VERSION	1
#define IMA_ATTESTATION_KEXEC_SIZE	SZ_1K
#define IMA_ATTESTATION_KEXEC_NAME	"ima-attestation"

#define IMA_ATTESTATION_SELECTOR_FAMILY	BIT(0)
#define IMA_ATTESTATION_SELECTOR_BUS	BIT(1)
#define IMA_ATTESTATION_SELECTOR_DEVICE	BIT(2)

static const char ima_attestation_record_prefix[] =
	"attestation-mr-record-v1";

struct ima_attestation_kexec_writer {
	u8 *data;
	size_t length;
	size_t capacity;
};

struct ima_attestation_kexec_reader {
	const u8 *data;
	size_t length;
	size_t offset;
};

static void *ima_attestation_kexec_out;
static const void *ima_attestation_kexec_in;
static const void *ima_attestation_kexec_consumed;
static size_t ima_attestation_kexec_in_size;
static bool ima_attestation_kexec_registered;

struct ima_attestation_file {
	/* Serializes read and seek on one open file. */
	struct mutex lock;
	struct attestation_mr_target *target;
	u8 *scratch;
	loff_t position;
	unsigned int generation;
	unsigned int end_generation;
	unsigned int visited;
	unsigned int mr_id;
	int srcu_index;
	enum hash_algo algorithm;
	bool srcu_active;
};

static int
ima_attestation_kexec_put(struct ima_attestation_kexec_writer *writer,
			  const void *data, size_t length)
{
	if (length > writer->capacity - writer->length)
		return -E2BIG;
	if (length)
		memcpy(writer->data + writer->length, data, length);
	writer->length += length;
	return 0;
}

static int
ima_attestation_kexec_put_u32(struct ima_attestation_kexec_writer *writer,
			      u32 value)
{
	__le32 encoded = cpu_to_le32(value);

	return ima_attestation_kexec_put(writer, &encoded, sizeof(encoded));
}

static int
ima_attestation_kexec_put_u64(struct ima_attestation_kexec_writer *writer,
			      u64 value)
{
	__le64 encoded = cpu_to_le64(value);

	return ima_attestation_kexec_put(writer, &encoded, sizeof(encoded));
}

static void
ima_attestation_kexec_patch_u32(struct ima_attestation_kexec_writer *writer,
				size_t offset, u32 value)
{
	__le32 encoded = cpu_to_le32(value);

	if (!WARN_ON_ONCE(offset > writer->length - sizeof(encoded)))
		memcpy(writer->data + offset, &encoded, sizeof(encoded));
}

static int
ima_attestation_kexec_get(struct ima_attestation_kexec_reader *reader,
			  void *data, size_t length)
{
	if (length > reader->length - reader->offset)
		return -EINVAL;
	memcpy(data, reader->data + reader->offset, length);
	reader->offset += length;
	return 0;
}

static int
ima_attestation_kexec_get_u32(struct ima_attestation_kexec_reader *reader,
			      u32 *value)
{
	__le32 encoded;
	int ret;

	ret = ima_attestation_kexec_get(reader, &encoded, sizeof(encoded));
	if (!ret)
		*value = le32_to_cpu(encoded);
	return ret;
}

static int
ima_attestation_kexec_get_u64(struct ima_attestation_kexec_reader *reader,
			      u64 *value)
{
	__le64 encoded;
	int ret;

	ret = ima_attestation_kexec_get(reader, &encoded, sizeof(encoded));
	if (!ret)
		*value = le64_to_cpu(encoded);
	return ret;
}

static bool
ima_attestation_kexec_string_equal(struct ima_attestation_kexec_reader *reader,
				   u32 length, const char *expected)
{
	size_t expected_length = expected ? strlen(expected) : 0;
	bool equal;

	if (length > reader->length - reader->offset)
		return false;
	equal = length == expected_length &&
		(!length ||
		 !memcmp(reader->data + reader->offset, expected, length));
	reader->offset += length;
	return equal;
}

static void __init ima_attestation_set_error(int error)
{
	if (!ima_selector.error)
		ima_selector.error = error;
}

static int __init ima_attestation_string_setup(char *value, char *field,
					       size_t field_size, bool *present)
{
	ssize_t length;

	if (*present)
		return -EINVAL;
	*present = true;
	if (!*value)
		return -EINVAL;
	length = strscpy(field, value, field_size);
	if (length < 0)
		return -E2BIG;
	return 0;
}

static int __init ima_attestation_setup(char *value)
{
	const char *mr_id = value;
	const char *separator;
	size_t family_length;
	int ret;

	if (ima_selector.target_present) {
		ima_attestation_set_error(-EINVAL);
		return 1;
	}
	ima_selector.target_present = true;
	if (!*value) {
		ima_attestation_set_error(-EINVAL);
		return 1;
	}

	separator = strchr(value, ':');
	if (separator) {
		if (separator == value || !separator[1] ||
		    strchr(separator + 1, ':')) {
			ret = -EINVAL;
			goto done;
		}
		family_length = separator - value;
		if (family_length >= sizeof(ima_attestation_boot_family)) {
			ret = -E2BIG;
			goto done;
		}
		memcpy(ima_attestation_boot_family, value, family_length);
		ima_attestation_boot_family[family_length] = '\0';
		mr_id = separator + 1;
	}

	ret = kstrtouint(mr_id, 10, &ima_selector.mr_id);
done:
	ima_attestation_set_error(ret);
	return 1;
}
__setup("ima_attestation=", ima_attestation_setup);

static int __init ima_attestation_bus_setup(char *value)
{
	int ret;

	ret = ima_attestation_string_setup(value, ima_attestation_boot_bus,
					   sizeof(ima_attestation_boot_bus),
					   &ima_selector.bus_present);
	ima_attestation_set_error(ret);
	return 1;
}
__setup("ima_attestation_bus=", ima_attestation_bus_setup);

static int __init ima_attestation_device_setup(char *value)
{
	int ret;

	ret = ima_attestation_string_setup(value, ima_attestation_boot_device,
					   sizeof(ima_attestation_boot_device),
					   &ima_selector.device_present);
	ima_attestation_set_error(ret);
	return 1;
}
__setup("ima_attestation_dev=", ima_attestation_device_setup);

bool ima_attestation_requested(void)
{
	return ima_selector.target_present || ima_selector.bus_present ||
	       ima_selector.device_present || ima_selector.error;
}

bool ima_attestation_mode(void)
{
	return ima_attestation_target;
}

static int __init ima_attestation_selector_validate(void)
{
	if (ima_selector.error)
		return ima_selector.error;
	if (!ima_attestation_requested())
		return 0;
	if (!ima_selector.target_present)
		return -EINVAL;
	if (ima_selector.bus_present != ima_selector.device_present)
		return -EINVAL;
	if (!ima_attestation_boot_family[0] && !ima_selector.bus_present)
		return -EINVAL;
	return 0;
}

static int __init ima_attestation_selector_materialize(void)
{
	if (ima_attestation_boot_family[0]) {
		ima_selector.family_name =
			kstrdup(ima_attestation_boot_family, GFP_KERNEL);
		if (!ima_selector.family_name)
			return -ENOMEM;
	}
	if (ima_selector.bus_present) {
		ima_selector.bus_name =
			kstrdup(ima_attestation_boot_bus, GFP_KERNEL);
		ima_selector.device_name =
			kstrdup(ima_attestation_boot_device, GFP_KERNEL);
		if (!ima_selector.bus_name || !ima_selector.device_name)
			return -ENOMEM;
	}
	return 0;
}

static int ima_attestation_kexec_capture(void)
{
	struct ima_attestation_kexec_writer writer = {
		.data = ima_attestation_kexec_out,
		.capacity = IMA_ATTESTATION_KEXEC_SIZE,
	};
	struct attestation_mr_target *target =
		ima_attestation_target->mr_target;
	const char *resolved_family =
		ima_attestation_target->resolved_family;
	const char *resolved_bus = ima_attestation_target->resolved_bus;
	const char *resolved_device =
		ima_attestation_target->resolved_device;
	u32 family_length = ima_selector.family_name ?
		strlen(ima_selector.family_name) : 0;
	u32 bus_length = ima_selector.bus_name ?
		strlen(ima_selector.bus_name) : 0;
	u32 device_length = ima_selector.device_name ?
		strlen(ima_selector.device_name) : 0;
	u32 resolved_family_length = strlen(resolved_family);
	u32 resolved_bus_length = strlen(resolved_bus);
	u32 resolved_device_length = strlen(resolved_device);
	u32 flags = 0;
	unsigned int end_generation;
	unsigned int start_generation;
	long violations;
	int ret;

	ret = attestation_mr_log_end_generation(target, &end_generation);
	if (ret)
		return ret;
	start_generation =
		attestation_mr_target_start_generation(target);
	violations = atomic_long_read(&ima_num_violations);
	if (violations < 0)
		return -EOVERFLOW;
	if (ima_selector.family_name)
		flags |= IMA_ATTESTATION_SELECTOR_FAMILY;
	if (ima_selector.bus_name)
		flags |= IMA_ATTESTATION_SELECTOR_BUS;
	if (ima_selector.device_name)
		flags |= IMA_ATTESTATION_SELECTOR_DEVICE;

	memset(writer.data, 0, writer.capacity);
	ret = ima_attestation_kexec_put_u32(&writer,
					    IMA_ATTESTATION_KEXEC_MAGIC);
	ret |= ima_attestation_kexec_put_u32(&writer,
					     IMA_ATTESTATION_KEXEC_VERSION);
	ret |= ima_attestation_kexec_put_u32(&writer, 0);
	ret |= ima_attestation_kexec_put_u32(&writer, 0);
	ret |= ima_attestation_kexec_put_u32(&writer, flags);
	ret |= ima_attestation_kexec_put_u32(&writer,
					     ima_attestation_target->mr_id);
	ret |= ima_attestation_kexec_put_u32(&writer, start_generation);
	ret |= ima_attestation_kexec_put_u32(&writer, end_generation);
	ret |= ima_attestation_kexec_put_u64(&writer, violations);
	ret |= ima_attestation_kexec_put_u32(&writer, family_length);
	ret |= ima_attestation_kexec_put_u32(&writer, bus_length);
	ret |= ima_attestation_kexec_put_u32(&writer, device_length);
	ret |= ima_attestation_kexec_put_u32(&writer,
					     resolved_family_length);
	ret |= ima_attestation_kexec_put_u32(&writer, resolved_bus_length);
	ret |= ima_attestation_kexec_put_u32(&writer,
					     resolved_device_length);
	ret |= ima_attestation_kexec_put(&writer, ima_selector.family_name,
					 family_length);
	ret |= ima_attestation_kexec_put(&writer, ima_selector.bus_name,
					 bus_length);
	ret |= ima_attestation_kexec_put(&writer, ima_selector.device_name,
					 device_length);
	ret |= ima_attestation_kexec_put(&writer, resolved_family,
					 resolved_family_length);
	ret |= ima_attestation_kexec_put(&writer, resolved_bus,
					 resolved_bus_length);
	ret |= ima_attestation_kexec_put(&writer, resolved_device,
					 resolved_device_length);
	if (ret)
		return ret;
	ima_attestation_kexec_patch_u32(&writer, 12, writer.length);
	/* Publish completeness only after the full payload. */
	smp_wmb();
	ima_attestation_kexec_patch_u32(&writer, 8, 1);
	return 0;
}

void ima_attestation_measure_kexec_event(const char *event_name)
{
	char event[48];
	int length;

	length = scnprintf(event, sizeof(event), "mr-id=%u;",
			   ima_attestation_target->mr_id);
	ima_measure_critical_data("ima_kexec", event_name, event, length,
				  false, NULL, 0);
}

static int ima_attestation_kexec_notify(struct notifier_block *nb,
					unsigned long action, void *data)
{
	int ret;

	if (action != SYS_RESTART || !data ||
	    strcmp(data, "kexec reboot") || !ima_attestation_kexec_out)
		return NOTIFY_DONE;
	ret = ima_attestation_kexec_capture();
	if (ret) {
		pr_err("IMA attestation handover capture failed: %d\n", ret);
		memset(ima_attestation_kexec_out, 0,
		       IMA_ATTESTATION_KEXEC_SIZE);
	}
	return NOTIFY_DONE;
}

static struct notifier_block ima_attestation_kexec_nb = {
	.notifier_call = ima_attestation_kexec_notify,
	.priority = -2,
};

static int __init ima_attestation_kexec_input_init(void)
{
	phys_addr_t phys;
	size_t size;
	int ret;

	ret = kho_retrieve_subtree(IMA_ATTESTATION_KEXEC_NAME, &phys, &size);
	if (ret == -ENOENT || ret == -EOPNOTSUPP)
		return 0;
	if (ret)
		return ret;
	ima_attestation_kexec_in = phys_to_virt(phys);
	ima_attestation_kexec_in_size = size;
	if (size != IMA_ATTESTATION_KEXEC_SIZE) {
		kho_restore_free((void *)ima_attestation_kexec_in);
		ima_attestation_kexec_in = NULL;
		ima_attestation_kexec_in_size = 0;
		return -EINVAL;
	}
	return 0;
}

static int __init ima_attestation_kexec_output_init(void)
{
	int ret;

	if (!ima_attestation_target->resolved_family ||
	    !ima_attestation_target->resolved_bus ||
	    !ima_attestation_target->resolved_device)
		return -EOPNOTSUPP;
	if (!kho_is_enabled())
		return 0;
	ima_attestation_kexec_out =
		kho_alloc_preserve(IMA_ATTESTATION_KEXEC_SIZE);
	if (IS_ERR(ima_attestation_kexec_out)) {
		ret = PTR_ERR(ima_attestation_kexec_out);
		ima_attestation_kexec_out = NULL;
		return ret;
	}
	memset(ima_attestation_kexec_out, 0, IMA_ATTESTATION_KEXEC_SIZE);
	ret = kho_add_subtree(IMA_ATTESTATION_KEXEC_NAME,
			      ima_attestation_kexec_out,
			      IMA_ATTESTATION_KEXEC_SIZE);
	if (ret)
		goto err_buffer;
	ret = register_reboot_notifier(&ima_attestation_kexec_nb);
	if (ret)
		goto err_subtree;
	ima_attestation_kexec_registered = true;
	return 0;

err_subtree:
	kho_remove_subtree(ima_attestation_kexec_out);
err_buffer:
	kho_unpreserve_free(ima_attestation_kexec_out);
	ima_attestation_kexec_out = NULL;
	return ret;
}

int __init ima_attestation_init(void)
{
	struct ima_attestation_target *target;
	int ret;

	ret = ima_attestation_kexec_input_init();
	if (ret)
		return ret;
#ifdef CONFIG_HAVE_IMA_KEXEC
	if (ima_attestation_requested())
		ima_free_kexec_buffer();
#endif
	ret = ima_attestation_selector_validate();
	if (ret)
		return ret;
	if (!ima_attestation_requested())
		return ima_attestation_kexec_in ? -ESTALE : 0;
	ret = ima_attestation_selector_materialize();
	if (ret)
		return ret;

	target = kzalloc_obj(*target);
	if (!target)
		return -ENOMEM;
	target->mr_target =
		attestation_mr_target_get(ima_selector.family_name,
					  ima_selector.bus_name,
					  ima_selector.device_name,
					  ima_selector.mr_id);
	if (IS_ERR(target->mr_target)) {
		ret = PTR_ERR(target->mr_target);
		goto err_free_target;
	}
	target->algorithms =
		attestation_mr_target_algorithms(target->mr_target,
						 &target->algorithms_count);
	if (IS_ERR(target->algorithms)) {
		ret = PTR_ERR(target->algorithms);
		goto err_put_target;
	}
	ret = attestation_mr_target_identity(target->mr_target,
					     &target->resolved_family,
					     &target->resolved_bus,
					     &target->resolved_device);
	if (ret && ret != -EOPNOTSUPP)
		goto err_algorithms;
	ret = attestation_mr_target_handover_restored(target->mr_target);
	if (ret < 0)
		goto err_identity;
	target->handover_restored = ret;
	target->mr_id = ima_selector.mr_id;
	ima_attestation_target = target;
	ret = ima_attestation_kexec_output_init();
	if (ret == -EOPNOTSUPP)
		pr_info("IMA attestation handover needs a stable backing device\n");
	else if (ret)
		pr_err("IMA attestation handover reservation failed: %d\n",
		       ret);
	return 0;

err_identity:
	kfree(target->resolved_family);
	kfree(target->resolved_bus);
	kfree(target->resolved_device);
err_algorithms:
	kfree(target->algorithms);
err_put_target:
	attestation_mr_target_put(target->mr_target);
err_free_target:
	kfree(target);
	return ret;
}

int __init ima_attestation_projection_init(void)
{
	struct attestation_mr_target *target;

	if (!ima_attestation_mode())
		return 0;
	target = attestation_mr_target_clone(ima_attestation_target->mr_target);
	if (IS_ERR(target))
		return PTR_ERR(target);
	ima_attestation_target->projection_target = target;
	return 0;
}

void ima_attestation_destroy(void)
{
	struct ima_attestation_target *target = ima_attestation_target;
	struct ima_attestation_digest *digest;
	struct hlist_node *tmp;
	unsigned int bucket;

	if (ima_attestation_kexec_registered) {
		unregister_reboot_notifier(&ima_attestation_kexec_nb);
		ima_attestation_kexec_registered = false;
	}
	if (ima_attestation_kexec_out) {
		kho_remove_subtree(ima_attestation_kexec_out);
		kho_unpreserve_free(ima_attestation_kexec_out);
		ima_attestation_kexec_out = NULL;
	}
	if (ima_attestation_kexec_in) {
		kho_restore_free((void *)ima_attestation_kexec_in);
		ima_attestation_kexec_in = NULL;
		ima_attestation_kexec_in_size = 0;
	}
	if (ima_attestation_kexec_consumed) {
		kho_restore_free((void *)ima_attestation_kexec_consumed);
		ima_attestation_kexec_consumed = NULL;
	}
	ima_attestation_target = NULL;
	hash_for_each_safe(ima_attestation_digests, bucket, tmp, digest, node) {
		hash_del(&digest->node);
		kfree(digest);
	}
	if (target) {
		attestation_mr_target_put(target->projection_target);
		kfree(target->algorithms);
		kfree(target->resolved_family);
		kfree(target->resolved_bus);
		kfree(target->resolved_device);
		attestation_mr_target_put(target->mr_target);
		kfree(target);
	}
	kfree(ima_selector.family_name);
	kfree(ima_selector.bus_name);
	kfree(ima_selector.device_name);
	ima_selector.family_name = NULL;
	ima_selector.bus_name = NULL;
	ima_selector.device_name = NULL;
}

unsigned int ima_attestation_mr_id(void)
{
	return ima_attestation_target->mr_id;
}

static bool
ima_attestation_digest_exists(const struct attestation_digest_set *values)
{
	struct ima_attestation_digest *digest;
	unsigned int key = ima_hash_key(values->values[0].data);

	hash_for_each_possible(ima_attestation_digests, digest, node, key)
		if (!memcmp(digest->values, values->values,
			    sizeof(*values->values) * values->values_count))
			return true;
	return false;
}

static int
ima_attestation_values_alloc(const struct ima_template_entry *entry,
			     bool violation,
			     struct attestation_digest_set *values)
{
	size_t index;

	values->values_count = ima_attestation_target->algorithms_count;
	values->values = kcalloc(values->values_count, sizeof(*values->values),
				 GFP_NOFS);
	if (!values->values)
		return -ENOMEM;
	for (index = 0; index < values->values_count; index++) {
		enum hash_algo algorithm =
			ima_attestation_target->algorithms[index];

		values->values[index].algorithm = algorithm;
		if (violation)
			memset(values->values[index].data, 0xff,
			       hash_digest_size[algorithm]);
		else
			memcpy(values->values[index].data,
			       entry->digests[index].digest,
			       hash_digest_size[algorithm]);
	}
	return 0;
}

static int
ima_attestation_template_copy(const struct ima_template_entry *entry,
			      u8 **data, size_t *data_length)
{
	const struct ima_template_desc *desc = entry->template_desc;
	bool legacy = !strcmp(desc->name, IMA_TEMPLATE_IMA_NAME);
	size_t length = 0;
	size_t offset = 0;
	int index;

	for (index = 0; index < desc->num_fields; index++) {
		const struct ima_template_field *field = desc->fields[index];
		const struct ima_field_data *field_data =
			&entry->template_data[index];
		size_t field_length = field_data->len;
		bool length_prefix = true;

		if (legacy && !strcmp(field->field_id, "d"))
			length_prefix = false;
		else if (legacy && !strcmp(field->field_id, "n"))
			field_length = strlen((char *)field_data->data);
		if ((length_prefix &&
		     check_add_overflow(length, sizeof(u32), &length)) ||
		    check_add_overflow(length, field_length, &length) ||
		    length > 65536)
			return -EINVAL;
	}

	*data = length ? kmalloc(length, GFP_NOFS) : NULL;
	if (length && !*data)
		return -ENOMEM;
	for (index = 0; index < desc->num_fields; index++) {
		const struct ima_template_field *field = desc->fields[index];
		const struct ima_field_data *field_data =
			&entry->template_data[index];
		size_t field_length = field_data->len;
		bool length_prefix = true;

		if (legacy && !strcmp(field->field_id, "d"))
			length_prefix = false;
		else if (legacy && !strcmp(field->field_id, "n"))
			field_length = strlen((char *)field_data->data);
		if (length_prefix) {
			u32 encoded_length = (u32)field_length;

			if (ima_canonical_fmt)
				encoded_length = cpu_to_le32(encoded_length);
			memcpy(*data + offset, &encoded_length,
			       sizeof(encoded_length));
			offset += sizeof(encoded_length);
		}
		memcpy(*data + offset, field_data->data, field_length);
		offset += field_length;
	}
	*data_length = length;
	return 0;
}

int ima_attestation_store(struct ima_template_entry *entry, bool violation)
{
	struct ima_attestation_digest *digest = NULL;
	struct attestation_digest_set values = {};
	struct attestation_mr_target *target;
	const char *template_name;
	size_t template_data_length;
	u8 *template_data;
	size_t allocation_size;
	unsigned int key;
	int ret;

	ret = ima_attestation_values_alloc(entry, violation, &values);
	if (ret)
		return ret;
	if (!violation && !IS_ENABLED(CONFIG_IMA_DISABLE_HTABLE)) {
		if (ima_attestation_digest_exists(&values)) {
			ima_free_template_entry(entry);
			ret = -EEXIST;
			goto out_values;
		}
		allocation_size =
			struct_size(digest, values, values.values_count);
		digest = kmalloc(allocation_size, GFP_NOFS);
		if (!digest) {
			ret = -ENOMEM;
			goto out_values;
		}
		memcpy(digest->values, values.values,
		       sizeof(*values.values) * values.values_count);
	}
	ret = ima_attestation_template_copy(entry, &template_data,
					    &template_data_length);
	if (ret)
		goto out_digest;
	template_name = entry->template_desc->name[0] ?
		entry->template_desc->name : entry->template_desc->fmt;
	target = ima_attestation_target->mr_target;
	ret = attestation_mr_target_extend(target, &values, template_name,
					   template_data, template_data_length,
					   violation);
	kfree(template_data);
	if (ret)
		goto out_digest;
	if (digest) {
		key = ima_hash_key(values.values[0].data);
		hash_add(ima_attestation_digests, &digest->node, key);
	}
	atomic_long_inc(&ima_num_records[BINARY]);
	atomic_long_inc(&ima_num_records[BINARY_FULL]);
	ima_free_template_entry(entry);
	kfree(values.values);
	return 0;

out_digest:
	kfree(digest);
out_values:
	kfree(values.values);
	return ret;
}

static void ima_attestation_srcu_lock(struct ima_attestation_file *file)
{
	file->srcu_index = srcu_read_lock(&attestation_instances_srcu);
	file->srcu_active = true;
	file->visited = 0;
}

static void ima_attestation_srcu_unlock(struct ima_attestation_file *file)
{
	if (!file->srcu_active)
		return;
	srcu_read_unlock(&attestation_instances_srcu, file->srcu_index);
	file->srcu_active = false;
}

static const struct attestation_mr_log_entry *
ima_attestation_reseek(struct ima_attestation_file *file)
{
	const struct attestation_mr_log_entry *entry;

	ima_attestation_srcu_unlock(file);
	cond_resched();
	ima_attestation_srcu_lock(file);
	entry = attestation_mr_log_iter_seek(file->target, file->generation,
					     file->end_generation);
	if (IS_ERR(entry))
		ima_attestation_srcu_unlock(file);
	return entry;
}

static const struct attestation_mr_log_entry *
ima_attestation_after(struct ima_attestation_file *file,
		      const struct attestation_mr_log_entry *entry)
{
	if (file->visited >= IMA_ATTESTATION_BATCH_ENTRIES)
		return ima_attestation_reseek(file);
	return attestation_mr_log_iter_next(entry, file->end_generation);
}

static bool
ima_attestation_entry_selected(struct ima_attestation_file *file,
			       const struct attestation_mr_log_entry *entry)
{
	return attestation_mr_log_entry_selected(file->target, entry) &&
	       attestation_mr_log_entry_event(entry) ==
		       CTLV_ATTESTATION_MR_LOG_EVENT_EXTEND;
}

static const struct attestation_mr_log_entry *
ima_attestation_find_selected(struct ima_attestation_file *file,
			      const struct attestation_mr_log_entry *entry)
{
	while (!IS_ERR_OR_NULL(entry)) {
		file->visited++;
		if (ima_attestation_entry_selected(file, entry))
			return entry;
		file->generation =
			attestation_mr_log_entry_generation(entry);
		entry = ima_attestation_after(file, entry);
	}
	if (IS_ERR(entry))
		ima_attestation_srcu_unlock(file);
	return entry;
}

static void *ima_attestation_measurements_start(struct seq_file *seq,
						loff_t *pos)
{
	struct ima_attestation_file *file = seq->private;
	const struct attestation_mr_log_entry *entry;
	loff_t position;

	ima_attestation_srcu_unlock(file);
	if (*pos == file->position) {
		position = file->position;
	} else {
		file->generation = 0;
		position = 0;
	}

	ima_attestation_srcu_lock(file);
	entry = attestation_mr_log_iter_seek(file->target, file->generation,
					     file->end_generation);
	entry = ima_attestation_find_selected(file, entry);
	while (!IS_ERR_OR_NULL(entry) && position < *pos) {
		file->generation =
			attestation_mr_log_entry_generation(entry);
		position++;
		entry = ima_attestation_after(file, entry);
		entry = ima_attestation_find_selected(file, entry);
	}
	file->position = position;
	if (!entry)
		ima_attestation_srcu_unlock(file);
	return (void *)entry;
}

static void *ima_attestation_measurements_next(struct seq_file *seq, void *v,
					       loff_t *pos)
{
	struct ima_attestation_file *file = seq->private;
	const struct attestation_mr_log_entry *entry = v;

	file->generation = attestation_mr_log_entry_generation(entry);
	(*pos)++;
	file->position = *pos;
	entry = ima_attestation_after(file, entry);
	entry = ima_attestation_find_selected(file, entry);
	if (!entry)
		ima_attestation_srcu_unlock(file);
	return (void *)entry;
}

static void ima_attestation_measurements_stop(struct seq_file *seq, void *v)
{
	struct ima_attestation_file *file = seq->private;

	ima_attestation_srcu_unlock(file);
}

static const struct attestation_digest *
ima_attestation_entry_value(struct ima_attestation_file *file,
			    const struct attestation_mr_log_entry *entry)
{
	const struct attestation_digest *values;
	size_t values_count;
	size_t index;

	values = attestation_mr_log_entry_values(entry, &values_count);
	for (index = 0; index < values_count; index++)
		if (values[index].algorithm == file->algorithm)
			return &values[index];
	return NULL;
}

static void ima_attestation_put_u32(struct seq_file *seq, u32 value)
{
	if (ima_canonical_fmt)
		value = (__force u32)cpu_to_le32(value);
	ima_putc(seq, &value, sizeof(value));
}

static int
ima_attestation_record_encode(struct ima_attestation_file *file,
			      const struct attestation_mr_log_entry *entry,
			      size_t *encoded_length)
{
	const char *format =
		attestation_mr_log_entry_record_format(entry);
	const u8 *data;
	size_t format_length;
	size_t data_length;
	size_t length;
	size_t offset = 0;
	__le32 encoded;

	if (!format) {
		*encoded_length = 0;
		return 0;
	}
	data = attestation_mr_log_entry_record_data(entry, &data_length);
	format_length = strlen(format);
	length = sizeof(ima_attestation_record_prefix) + sizeof(encoded) +
		 format_length + sizeof(encoded) + data_length;
	if (format_length > IMA_ATTESTATION_RECORD_FORMAT_MAX ||
	    data_length > IMA_ATTESTATION_RECORD_DATA_MAX ||
	    length > IMA_ATTESTATION_TEMPLATE_MAX)
		return -EINVAL;

	memcpy(file->scratch + offset, ima_attestation_record_prefix,
	       sizeof(ima_attestation_record_prefix));
	offset += sizeof(ima_attestation_record_prefix);
	encoded = cpu_to_le32(format_length);
	memcpy(file->scratch + offset, &encoded, sizeof(encoded));
	offset += sizeof(encoded);
	memcpy(file->scratch + offset, format, format_length);
	offset += format_length;
	encoded = cpu_to_le32(data_length);
	memcpy(file->scratch + offset, &encoded, sizeof(encoded));
	offset += sizeof(encoded);
	if (data_length)
		memcpy(file->scratch + offset, data, data_length);
	*encoded_length = length;
	return 0;
}

static int
ima_attestation_fields_show(struct seq_file *seq,
			    struct ima_attestation_file *file,
			    const char *template_name,
			    const u8 *data, size_t data_length)
{
	struct ima_template_desc *desc = lookup_template_desc(template_name);
	bool legacy = !strcmp(template_name, IMA_TEMPLATE_IMA_NAME);
	size_t offset = 0;
	int index;

	if (!desc || !desc->fields || !desc->num_fields)
		return -EINVAL;
	for (index = 0; index < desc->num_fields; index++) {
		const struct ima_template_field *field = desc->fields[index];
		struct ima_field_data field_data;
		u32 field_length;

		if (legacy && !strcmp(field->field_id, "d")) {
			field_length = IMA_DIGEST_SIZE;
		} else {
			u32 encoded_length;

			if (data_length - offset < sizeof(encoded_length))
				return -EINVAL;
			memcpy(&encoded_length, data + offset,
			       sizeof(encoded_length));
			offset += sizeof(encoded_length);
			field_length = ima_canonical_fmt ?
				le32_to_cpu((__force __le32)encoded_length) :
				encoded_length;
		}
		if (field_length > data_length - offset ||
		    field_length >= IMA_ATTESTATION_TEMPLATE_MAX)
			return -EINVAL;
		memcpy(file->scratch, data + offset, field_length);
		file->scratch[field_length] = '\0';
		field_data.data = file->scratch;
		field_data.len = field_length;
		seq_puts(seq, " ");
		if (field_length)
			field->field_show(seq, IMA_SHOW_ASCII, &field_data);
		offset += field_length;
	}
	return offset == data_length ? 0 : -EINVAL;
}

static int ima_attestation_binary_show(struct seq_file *seq, void *v)
{
	struct ima_attestation_file *file = seq->private;
	const struct attestation_mr_log_entry *entry = v;
	const struct attestation_digest *value;
	const char *template_name;
	const u8 *template_data;
	size_t data_length;
	size_t record_length;
	u8 zero_digest[HASH_MAX_DIGESTSIZE] = {};
	u8 *digest;
	int ret;

	value = ima_attestation_entry_value(file, entry);
	if (!value)
		return -EINVAL;
	template_name =
		attestation_mr_log_entry_template_name(entry);
	if (template_name) {
		template_data =
			attestation_mr_log_entry_template_data(entry,
							       &data_length);
		digest = attestation_mr_log_entry_violation(entry) ?
			zero_digest : (u8 *)value->data;
	} else {
		template_name = "ima-mr-extend";
		digest = (u8 *)value->data;
		ret = ima_attestation_record_encode(file, entry,
						    &record_length);
		if (ret)
			return ret;
		template_data = file->scratch;
		data_length = record_length;
	}

	ima_attestation_put_u32(seq, file->mr_id);
	ima_putc(seq, digest, hash_digest_size[file->algorithm]);
	ima_attestation_put_u32(seq, strlen(template_name));
	ima_putc(seq, (void *)template_name, strlen(template_name));
	if (strcmp(template_name, IMA_TEMPLATE_IMA_NAME)) {
		ima_attestation_put_u32(seq, data_length);
		ima_putc(seq, (void *)template_data, data_length);
	}
	return 0;
}

static int ima_attestation_ascii_show(struct seq_file *seq, void *v)
{
	struct ima_attestation_file *file = seq->private;
	const struct attestation_mr_log_entry *entry = v;
	const struct attestation_digest *value;
	const char *template_name;
	const u8 *template_data;
	size_t data_length;
	size_t record_length;
	u8 zero_digest[HASH_MAX_DIGESTSIZE] = {};
	u8 *digest;
	int ret;

	value = ima_attestation_entry_value(file, entry);
	if (!value)
		return -EINVAL;
	template_name =
		attestation_mr_log_entry_template_name(entry);
	digest = (u8 *)value->data;
	if (template_name) {
		template_data =
			attestation_mr_log_entry_template_data(entry,
							       &data_length);
		if (attestation_mr_log_entry_violation(entry))
			digest = zero_digest;
	} else {
		template_name = "ima-mr-extend";
		ret = ima_attestation_record_encode(file, entry,
						    &record_length);
		if (ret)
			return ret;
		template_data = file->scratch;
		data_length = record_length;
	}

	seq_printf(seq, "%2u ", file->mr_id);
	ima_print_digest(seq, digest, hash_digest_size[file->algorithm]);
	seq_printf(seq, " %s", template_name);
	if (!strcmp(template_name, "ima-mr-extend")) {
		if (data_length) {
			seq_puts(seq, " ");
			ima_print_digest(seq, (u8 *)template_data,
					 data_length);
		}
	} else {
		ret = ima_attestation_fields_show(seq, file, template_name,
						  template_data, data_length);
		if (ret)
			return ret;
	}
	seq_puts(seq, "\n");
	return 0;
}

static const struct seq_operations ima_attestation_binary_seq_ops = {
	.start = ima_attestation_measurements_start,
	.next = ima_attestation_measurements_next,
	.stop = ima_attestation_measurements_stop,
	.show = ima_attestation_binary_show,
};

static const struct seq_operations ima_attestation_ascii_seq_ops = {
	.start = ima_attestation_measurements_start,
	.next = ima_attestation_measurements_next,
	.stop = ima_attestation_measurements_stop,
	.show = ima_attestation_ascii_show,
};

static int
ima_attestation_measurements_open(struct inode *inode, struct file *filp,
				  const struct seq_operations *seq_ops)
{
	struct ima_attestation_file *file;
	struct attestation_mr_target *projection;
	struct seq_file *seq;
	uintptr_t index = (uintptr_t)inode->i_private;
	int ret;

	if (index >= ima_algo_count)
		return -EINVAL;
	file = kzalloc_obj(*file);
	if (!file)
		return -ENOMEM;
	mutex_init(&file->lock);
	file->scratch = kmalloc(IMA_ATTESTATION_TEMPLATE_MAX + 1,
				GFP_KERNEL);
	if (!file->scratch) {
		ret = -ENOMEM;
		goto err_file;
	}
	projection = ima_attestation_target->projection_target;
	file->target = attestation_mr_target_clone(projection);
	if (IS_ERR(file->target)) {
		ret = PTR_ERR(file->target);
		goto err_scratch;
	}
	file->algorithm = ima_algo_array[index].algo;
	file->mr_id = ima_attestation_target->mr_id;
	ret = seq_open(filp, seq_ops);
	if (ret)
		goto err_target;
	seq = filp->private_data;
	seq->private = file;
	return 0;

err_target:
	attestation_mr_target_put(file->target);
err_scratch:
	kfree(file->scratch);
err_file:
	kfree(file);
	return ret;
}

static int ima_attestation_binary_open(struct inode *inode, struct file *filp)
{
	return ima_attestation_measurements_open(inode, filp,
					&ima_attestation_binary_seq_ops);
}

static int ima_attestation_ascii_open(struct inode *inode, struct file *filp)
{
	return ima_attestation_measurements_open(inode, filp,
					&ima_attestation_ascii_seq_ops);
}

static ssize_t
ima_attestation_measurements_read(struct file *filp, char __user *buf,
				  size_t count, loff_t *ppos)
{
	struct seq_file *seq = filp->private_data;
	struct ima_attestation_file *file = seq->private;
	ssize_t ret;

	ret = mutex_lock_interruptible(&file->lock);
	if (ret)
		return ret;
	ret = attestation_mr_log_end_generation(file->target,
						&file->end_generation);
	if (!ret)
		ret = seq_read(filp, buf, count, ppos);
	mutex_unlock(&file->lock);
	return ret;
}

static loff_t ima_attestation_measurements_lseek(struct file *filp,
						 loff_t offset, int whence)
{
	struct seq_file *seq = filp->private_data;
	struct ima_attestation_file *file = seq->private;
	loff_t ret;

	ret = mutex_lock_interruptible(&file->lock);
	if (ret)
		return ret;
	ret = attestation_mr_log_end_generation(file->target,
						&file->end_generation);
	if (!ret)
		ret = seq_lseek(filp, offset, whence);
	mutex_unlock(&file->lock);
	return ret;
}

static int ima_attestation_measurements_release(struct inode *inode,
						struct file *filp)
{
	struct seq_file *seq = filp->private_data;
	struct ima_attestation_file *file = seq->private;
	int ret;

	ima_attestation_srcu_unlock(file);
	ret = seq_release(inode, filp);
	attestation_mr_target_put(file->target);
	kfree(file->scratch);
	kfree(file);
	return ret;
}

const struct file_operations ima_attestation_binary_ops = {
	.open = ima_attestation_binary_open,
	.read = ima_attestation_measurements_read,
	.llseek = ima_attestation_measurements_lseek,
	.release = ima_attestation_measurements_release,
};

const struct file_operations ima_attestation_ascii_ops = {
	.open = ima_attestation_ascii_open,
	.read = ima_attestation_measurements_read,
	.llseek = ima_attestation_measurements_lseek,
	.release = ima_attestation_measurements_release,
};

int ima_attestation_measurements_count(unsigned long *count)
{
	struct ima_attestation_file file = {
		.target = ima_attestation_target->projection_target,
	};
	const struct attestation_mr_log_entry *entry;
	int ret;

	*count = 0;
	ret = attestation_mr_log_end_generation(file.target,
						&file.end_generation);
	if (ret)
		return ret;
	ima_attestation_srcu_lock(&file);
	entry = attestation_mr_log_iter_seek(file.target, 0,
					     file.end_generation);
	entry = ima_attestation_find_selected(&file, entry);
	while (!IS_ERR_OR_NULL(entry)) {
		if (*count == ULONG_MAX) {
			ret = -EOVERFLOW;
			goto out;
		}
		(*count)++;
		file.generation =
			attestation_mr_log_entry_generation(entry);
		entry = ima_attestation_after(&file, entry);
		entry = ima_attestation_find_selected(&file, entry);
	}
	ret = IS_ERR(entry) ? PTR_ERR(entry) : 0;
out:
	ima_attestation_srcu_unlock(&file);
	return ret;
}

static int
ima_attestation_restore_digest(const struct attestation_digest_set *values)
{
	struct ima_attestation_digest *digest;
	size_t allocation_size;
	unsigned int key;

	if (IS_ENABLED(CONFIG_IMA_DISABLE_HTABLE) ||
	    ima_attestation_digest_exists(values))
		return 0;
	allocation_size = struct_size(digest, values, values->values_count);
	digest = kmalloc(allocation_size, GFP_KERNEL);
	if (!digest)
		return -ENOMEM;
	memcpy(digest->values, values->values,
	       sizeof(*values->values) * values->values_count);
	key = ima_hash_key(values->values[0].data);
	hash_add(ima_attestation_digests, &digest->node, key);
	return 0;
}

static int ima_attestation_restore_digests(unsigned int end_generation)
{
	struct ima_attestation_file file = {
		.target = ima_attestation_target->mr_target,
		.end_generation = end_generation,
	};
	const struct attestation_mr_log_entry *entry;
	unsigned long records = 0;
	int ret = 0;

	ima_attestation_srcu_lock(&file);
	entry = attestation_mr_log_iter_seek(file.target, 0,
					     file.end_generation);
	entry = ima_attestation_find_selected(&file, entry);
	while (!IS_ERR_OR_NULL(entry)) {
		const struct attestation_digest *digests;
		const struct ima_template_desc *template;
		const char *template_name;
		struct attestation_digest_set values;
		size_t count;
		size_t index;

		if (records == LONG_MAX) {
			ret = -EOVERFLOW;
			goto out;
		}
		records++;
		template_name =
			attestation_mr_log_entry_template_name(entry);
		template = template_name ?
			lookup_template_desc(template_name) : NULL;
		if (template_name &&
		    (!template || !template->fields || !template->num_fields)) {
			ret = -ESTALE;
			goto out;
		}
		if (template_name &&
		    !attestation_mr_log_entry_violation(entry)) {
			digests = attestation_mr_log_entry_values(entry,
								  &count);
			if (count !=
			    ima_attestation_target->algorithms_count) {
				ret = -EINVAL;
				goto out;
			}
			for (index = 0; index < count; index++)
				if (digests[index].algorithm !=
				    ima_attestation_target->algorithms[index]) {
					ret = -EINVAL;
					goto out;
				}
			values.values = (struct attestation_digest *)digests;
			values.values_count = count;
			ret = ima_attestation_restore_digest(&values);
			if (ret)
				goto out;
		}
		file.generation =
			attestation_mr_log_entry_generation(entry);
		entry = ima_attestation_after(&file, entry);
		entry = ima_attestation_find_selected(&file, entry);
	}
	ret = IS_ERR(entry) ? PTR_ERR(entry) : 0;
	if (!ret) {
		atomic_long_set(&ima_num_records[BINARY], records);
		atomic_long_set(&ima_num_records[BINARY_FULL], records);
	}
out:
	ima_attestation_srcu_unlock(&file);
	return ret;
}

static int
ima_attestation_kexec_header_read(struct ima_attestation_kexec_reader *reader,
				  unsigned int *end_generation,
				  u64 *violations)
{
	struct attestation_mr_target *target =
		ima_attestation_target->mr_target;
	const char *family = ima_selector.family_name;
	const char *bus = ima_selector.bus_name;
	const char *device = ima_selector.device_name;
	const char *resolved_family = ima_attestation_target->resolved_family;
	const char *resolved_bus = ima_attestation_target->resolved_bus;
	const char *resolved_device = ima_attestation_target->resolved_device;
	u32 magic, version, complete, length, flags;
	u32 mr_id, start_generation;
	u32 family_length, bus_length, device_length;
	u32 resolved_family_length, resolved_bus_length;
	u32 resolved_device_length;
	u32 expected_flags = 0;
	unsigned int current_start;
	bool family_match;
	bool device_match;
	bool bus_match;
	bool resolved_family_match;
	bool resolved_device_match;
	bool resolved_bus_match;
	int ret;

	ret = ima_attestation_kexec_get_u32(reader, &magic);
	ret |= ima_attestation_kexec_get_u32(reader, &version);
	ret |= ima_attestation_kexec_get_u32(reader, &complete);
	ret |= ima_attestation_kexec_get_u32(reader, &length);
	ret |= ima_attestation_kexec_get_u32(reader, &flags);
	ret |= ima_attestation_kexec_get_u32(reader, &mr_id);
	ret |= ima_attestation_kexec_get_u32(reader, &start_generation);
	ret |= ima_attestation_kexec_get_u32(reader, end_generation);
	ret |= ima_attestation_kexec_get_u64(reader, violations);
	ret |= ima_attestation_kexec_get_u32(reader, &family_length);
	ret |= ima_attestation_kexec_get_u32(reader, &bus_length);
	ret |= ima_attestation_kexec_get_u32(reader, &device_length);
	ret |= ima_attestation_kexec_get_u32(reader,
					     &resolved_family_length);
	ret |= ima_attestation_kexec_get_u32(reader, &resolved_bus_length);
	ret |= ima_attestation_kexec_get_u32(reader,
					     &resolved_device_length);
	if (ret || magic != IMA_ATTESTATION_KEXEC_MAGIC ||
	    version != IMA_ATTESTATION_KEXEC_VERSION || complete != 1 ||
	    length < reader->offset || length > reader->length ||
	    flags & ~(IMA_ATTESTATION_SELECTOR_FAMILY |
		      IMA_ATTESTATION_SELECTOR_BUS |
		      IMA_ATTESTATION_SELECTOR_DEVICE) ||
	    !!(flags & IMA_ATTESTATION_SELECTOR_BUS) !=
		    !!(flags & IMA_ATTESTATION_SELECTOR_DEVICE) ||
	    !start_generation || start_generation > *end_generation ||
	    *violations > LONG_MAX)
		return -EINVAL;
	reader->length = length;

	if (family)
		expected_flags |= IMA_ATTESTATION_SELECTOR_FAMILY;
	if (bus)
		expected_flags |= IMA_ATTESTATION_SELECTOR_BUS;
	if (device)
		expected_flags |= IMA_ATTESTATION_SELECTOR_DEVICE;
	current_start = attestation_mr_target_start_generation(target);
	if (flags != expected_flags || mr_id != ima_selector.mr_id ||
	    start_generation != current_start)
		return -ESTALE;
	family_match = ima_attestation_kexec_string_equal(reader,
							  family_length,
							  family);
	bus_match = ima_attestation_kexec_string_equal(reader, bus_length,
						       bus);
	device_match = ima_attestation_kexec_string_equal(reader,
							  device_length,
							  device);
	resolved_family_match =
		ima_attestation_kexec_string_equal(reader,
						   resolved_family_length,
						   resolved_family);
	resolved_bus_match =
		ima_attestation_kexec_string_equal(reader,
						   resolved_bus_length,
						   resolved_bus);
	resolved_device_match =
		ima_attestation_kexec_string_equal(reader,
						   resolved_device_length,
						   resolved_device);
	if (!family_match || !bus_match || !device_match ||
	    !resolved_family_match || !resolved_bus_match ||
	    !resolved_device_match ||
	    reader->offset != reader->length)
		return -ESTALE;
	return 0;
}

int __init ima_attestation_kexec_restore(void)
{
	struct ima_attestation_kexec_reader reader;
	struct attestation_mr_target *target;
	unsigned int current_end;
	unsigned int expected_end;
	unsigned int saved_end;
	u64 violations;
	int ret;

	if (!ima_attestation_mode())
		return 0;
	if (!ima_attestation_kexec_in)
		return is_kho_boot() ? -ESTALE : 0;
	if (!is_kho_boot()) {
		ret = -EINVAL;
		goto out_free;
	}
	if (!ima_attestation_target->handover_restored) {
		ret = -ESTALE;
		goto out_free;
	}
	reader = (struct ima_attestation_kexec_reader) {
		.data = ima_attestation_kexec_in,
		.length = ima_attestation_kexec_in_size,
	};
	ret = ima_attestation_kexec_header_read(&reader, &saved_end,
						&violations);
	if (ret)
		goto out_free;
	target = ima_attestation_target->mr_target;
	ret = attestation_mr_log_end_generation(target, &current_end);
	if (ret)
		goto out_free;
	if (check_add_overflow(saved_end, 1U, &expected_end) ||
	    current_end != expected_end) {
		ret = -ESTALE;
		goto out_free;
	}
	ret = ima_attestation_restore_digests(saved_end);
	if (!ret)
		atomic_long_set(&ima_num_violations, violations);

out_free:
	ima_attestation_kexec_consumed = ima_attestation_kexec_in;
	ima_attestation_kexec_in = NULL;
	ima_attestation_kexec_in_size = 0;
	return ret;
}

int ima_attestation_policy_check(const char *family_name,
				 const char *bus_name,
				 const char *device_name,
				 bool mr_id_present, unsigned int mr_id,
				 bool pcr_present)
{
	bool device_selector = bus_name || device_name;
	bool selector = family_name || device_selector || mr_id_present;

	if (!ima_attestation_mode())
		return selector ? -EINVAL : 0;
	if (pcr_present)
		return -EINVAL;
	if (!selector)
		return 0;
	if (!mr_id_present || (bus_name && !device_name) ||
	    (!bus_name && device_name) || (!family_name && !bus_name))
		return -EINVAL;
	return attestation_mr_target_check(ima_attestation_target->mr_target,
					   family_name, bus_name, device_name,
					   mr_id);
}

MODULE_IMPORT_NS("ATTESTATION");
