// SPDX-License-Identifier: GPL-2.0-only
#include <linux/attestation/ima.h>
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/overflow.h>
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

static struct ima_attestation_selector ima_selector __initdata;
static char ima_attestation_boot_family[256];
static char ima_attestation_boot_bus[256];
static char ima_attestation_boot_device[256];

struct ima_attestation_target *ima_attestation_target;

struct ima_attestation_digest {
	struct hlist_node node;
	struct attestation_digest values[];
};

static DEFINE_HASHTABLE(ima_attestation_digests, IMA_HASH_BITS);

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

int __init ima_attestation_init(void)
{
	struct ima_attestation_target *target;
	int ret;

	ret = ima_attestation_selector_validate();
	if (ret)
		return ret;
	if (!ima_attestation_requested())
		return 0;
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
	target->mr_id = ima_selector.mr_id;
	ima_attestation_target = target;
	return 0;

err_put_target:
	attestation_mr_target_put(target->mr_target);
err_free_target:
	kfree(target);
	return ret;
}

void __init ima_attestation_destroy(void)
{
	struct ima_attestation_target *target = ima_attestation_target;
	struct ima_attestation_digest *digest;
	struct hlist_node *tmp;
	unsigned int bucket;

	ima_attestation_target = NULL;
	hash_for_each_safe(ima_attestation_digests, bucket, tmp, digest, node) {
		hash_del(&digest->node);
		kfree(digest);
	}
	if (target) {
		kfree(target->algorithms);
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
