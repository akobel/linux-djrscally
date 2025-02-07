// SPDX-License-Identifier: GPL-2.0
/* Author: Dan Scally <djrscally@gmail.com> */

#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include "intel_skl_int3472_common.h"

int skl_int3472_get_cldb_buffer(struct acpi_device *adev,
				struct int3472_cldb *cldb)
{
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	acpi_handle handle = adev->handle;
	union acpi_object *obj;
	acpi_status status;
	int ret = 0;

	status = acpi_evaluate_object(handle, "CLDB", NULL, &buffer);
	if (ACPI_FAILURE(status))
		return -ENODEV;

	obj = buffer.pointer;
	if (!obj) {
		dev_err(&adev->dev, "ACPI device has no CLDB object\n");
		return -ENODEV;
	}

	if (obj->type != ACPI_TYPE_BUFFER) {
		dev_err(&adev->dev, "CLDB object is not an ACPI buffer\n");
		ret = -EINVAL;
		goto out_free_buff;
	}

	if (obj->buffer.length > sizeof(*cldb)) {
		dev_err(&adev->dev, "The CLDB buffer is too large\n");
		ret = -EINVAL;
		goto out_free_buff;
	}

	memcpy(cldb, obj->buffer.pointer, obj->buffer.length);

out_free_buff:
	kfree(buffer.pointer);
	return ret;
}

static const struct acpi_device_id int3472_device_id[] = {
	{ "INT3472", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, int3472_device_id);

static struct platform_driver int3472_discrete = {
	.driver = {
		.name = "int3472-discrete",
		.acpi_match_table = int3472_device_id,
	},
	.probe = skl_int3472_discrete_probe,
	.remove = skl_int3472_discrete_remove,
};

static struct i2c_driver int3472_tps68470 = {
	.driver = {
		.name = "int3472-tps68470",
		.acpi_match_table = int3472_device_id,
	},
	.probe_new = skl_int3472_tps68470_probe,
};

static int skl_int3472_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&int3472_discrete);
	if (ret)
		return ret;

	ret = i2c_register_driver(THIS_MODULE, &int3472_tps68470);
	if (ret)
		goto err_unregister_plat_drv;

	return 0;

err_unregister_plat_drv:
	platform_driver_unregister(&int3472_discrete);
	return ret;
}
module_init(skl_int3472_init);

static void skl_int3472_exit(void)
{
	platform_driver_unregister(&int3472_discrete);
	i2c_del_driver(&int3472_tps68470);
}
module_exit(skl_int3472_exit);

MODULE_DESCRIPTION("Intel SkyLake INT3472 ACPI Device Driver");
MODULE_AUTHOR("Daniel Scally <djrscally@gmail.com>");
MODULE_LICENSE("GPL v2");
