// SPDX-License-Identifier: GPL-2.0-only
#include <linux/attestation/ima.h>
#include <linux/init.h>
#include <linux/module.h>
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

	ima_attestation_target = NULL;
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
