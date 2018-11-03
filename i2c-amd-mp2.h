/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * AMD MP2 I2C adapter driver
 *
 * Authors: Shyam Sundar S K <Shyam-sundar.S-k@amd.com>
 *          Elie Morisse <syniurge@gmail.com>
 */

#ifndef I2C_AMD_PCI_MP2_H
#define I2C_AMD_PCI_MP2_H

#include <linux/pci.h>

#define PCI_DEVICE_ID_AMD_MP2	0x15E6

struct amd_i2c_common;
struct amd_mp2_dev;

enum {
	/* MP2 C2P Message Registers */
	AMD_C2P_MSG0 = 0x10500,			/* MP2 Message for I2C0 */
	AMD_C2P_MSG1 = 0x10504,			/* MP2 Message for I2C1 */
	AMD_C2P_MSG2 = 0x10508,			/* DRAM Address Lo / Data 0 */
	AMD_C2P_MSG3 = 0x1050c,			/* DRAM Address HI / Data 1 */
	AMD_C2P_MSG4 = 0x10510,			/* Data 2 */
	AMD_C2P_MSG5 = 0x10514,			/* Data 3 */
	AMD_C2P_MSG6 = 0x10518,			/* Data 4 */
	AMD_C2P_MSG7 = 0x1051c,			/* Data 5 */
	AMD_C2P_MSG8 = 0x10520,			/* Data 6 */
	AMD_C2P_MSG9 = 0x10524,			/* Data 7 */

	/* MP2 P2C Message Registers */
	AMD_P2C_MSG0 = 0x10680,			/* Do not use */
	AMD_P2C_MSG1 = 0x10684,			/* I2C0 interrupt register */
	AMD_P2C_MSG2 = 0x10688,			/* I2C1 interrupt register */
	AMD_P2C_MSG3 = 0x1068C,			/* MP2 debug info */
	AMD_P2C_MSG_INTEN = 0x10690,		/* MP2 interrupt gen register */
	AMD_P2C_MSG_INTSTS = 0x10694,		/* Interrupt status */
};

/* Command register data structures */

#define i2c_none (-1)
enum i2c_cmd {
	i2c_read = 0,
	i2c_write,
	i2c_enable,
	i2c_disable,
	number_of_sensor_discovered,
	is_mp2_active,
	invalid_cmd = 0xF,
};

enum speed_enum {
	speed100k = 0,
	speed400k = 1,
	speed1000k = 2,
	speed1400k = 3,
	speed3400k = 4
};

enum mem_type {
	use_dram = 0,
	use_c2pmsg = 1,
};

/**
 * union i2c_cmd_base : bit access of C2P commands
 * @i2c_cmd: bit 0..3 i2c R/W command
 * @bus_id: bit 4..7 i2c bus index
 * @slave_addr: bit 8..15 slave address
 * @length: bit 16..27 read/write length
 * @i2c_speed: bit 28..30 bus speed
 * @mem_type: bit 31 0-DRAM; 1-C2P msg o/p
 */
union i2c_cmd_base {
	u32 ul;
	struct {
		enum i2c_cmd i2c_cmd : 4;
		u8 bus_id : 4;
		u32 slave_addr : 8;
		u32 length : 12;
		enum speed_enum i2c_speed : 3;
		enum mem_type mem_type : 1;
	} s;
};

/* Response - Response of SFI */
enum response_type {
	invalid_response = 0,
	command_success = 1,
	command_failed = 2,
};

/* Status - Command ID to indicate a command */
enum status_type {
	i2c_readcomplete_event = 0,
	i2c_readfail_event = 1,
	i2c_writecomplete_event = 2,
	i2c_writefail_event = 3,
	i2c_busenable_complete = 4,
	i2c_busenable_failed = 5,
	i2c_busdisable_complete = 6,
	i2c_busdisable_failed = 7,
	invalid_data_length = 8,
	invalid_slave_address = 9,
	invalid_i2cbus_id = 10,
	invalid_dram_addr = 11,
	invalid_command = 12,
	mp2_active = 13,
	numberof_sensors_discovered_resp = 14,
	i2c_bus_notinitialized
};

/**
 * union i2c_event : bit access of P2C events
 * @response: bit 0..1 i2c response type
 * @status: bit 2..6 status_type
 * @mem_type: bit 7 0-DRAM; 1-C2P msg o/p
 * @bus_id: bit 8..11 i2c bus id
 * @length: bit 12..23 message length
 * @slave_addr: bit 24-31 slave address
 */
union i2c_event {
	u32 ul;
	struct {
		enum response_type response : 2;
		enum status_type status : 5;
		enum mem_type mem_type : 1;
		u8 bus_id : 4;
		u32 length : 12;
		u32 slave_addr : 8;
	} r;
};

/**
 * struct i2c_rw_config - i2c read/write settings
 * @slave_addr: slave address
 * @length: message length
 * @buf: buffer address
 * @dma_addr: if length > 32, holds the DMA buffer address
 * @dma_direction: if length > 32, is either FROM or TO device
 */
struct i2c_rw_config {
	u16 slave_addr;
	u32 length;
	u32 *buf;
	dma_addr_t dma_addr;
	enum dma_data_direction dma_direction;
};

/**
 * struct amd_i2c_common - per bus/i2c adapter context, shared
 *		between the pci and the platform driver
 * @eventval: MP2 event value set by the IRQ handler to be processed
 *		by the worker
 * @ops: platdrv hooks
 * @rw_cfg: settings for reads/writes
 * @work: delayed worker struct
 * @reqcmd: i2c command type requested by platdrv
 * @requested: true if the interrupt answered a request from platdrv
 * @bus_id: bus index
 * @i2c_speed: i2c bus speed determined by the slowest slave
 */
struct amd_i2c_common {
	union i2c_event eventval;
	struct amd_mp2_dev *mp2_dev;
	struct i2c_rw_config rw_cfg;
	struct delayed_work work;
	enum i2c_cmd reqcmd;
	u8 bus_id;
	enum speed_enum i2c_speed;
};

/**
 * struct amd_mp2_dev - per PCI device context
 * @pci_dev: PCI driver node
 * @plat_common: MP2 devices may have up to two busses,
 *		each bus corresponding to an i2c adapter
 * @mmio: iommapped registers
 * @lock: interrupt spinlock
 * @c2p_lock: controls access to the C2P mailbox shared between
 *	      the two adapters
 * @c2p_lock_busid: id of the adapter which locked c2p_lock
 */
struct amd_mp2_dev {
	struct pci_dev *pci_dev;
	struct amd_i2c_common *plat_common[2];
	void __iomem *mmio;
	raw_spinlock_t lock;
	struct mutex c2p_lock;
	u8 c2p_lock_busid;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
	struct dentry *debugfs_info;
#endif /* CONFIG_DEBUG_FS */
};

/* PCIe communication driver */

void amd_mp2_c2p_mutex_lock(struct amd_i2c_common *i2c_common);
void amd_mp2_c2p_mutex_unlock(struct amd_i2c_common *i2c_common);

int amd_mp2_read(struct amd_i2c_common *i2c_common);
int amd_mp2_write(struct amd_i2c_common *i2c_common);
int amd_mp2_connect(struct amd_i2c_common *i2c_common, bool enable);

int amd_i2c_register_cb(struct amd_mp2_dev *mp2_dev,
			struct amd_i2c_common *i2c_common);
int amd_i2c_unregister_cb(struct amd_mp2_dev *mp2_dev,
			  struct amd_i2c_common *i2c_common);

struct amd_mp2_dev *amd_mp2_find_device(struct pci_dev *candidate);

/* Platform driver */

void i2c_amd_read_completion(union i2c_event *event);
void i2c_amd_write_completion(union i2c_event *event);
void i2c_amd_connect_completion(union i2c_event *event);

int i2c_amd_register_driver(void);
void i2c_amd_unregister_driver(void);

#define ndev_pdev(ndev) ((ndev)->pci_dev)
#define ndev_name(ndev) pci_name(ndev_pdev(ndev))
#define ndev_dev(ndev) (&ndev_pdev(ndev)->dev)
#define work_amd_i2c_common(__work) \
	container_of(__work, struct amd_i2c_common, work.work)
#define event_amd_i2c_common(__event) \
	container_of(__event, struct amd_i2c_common, eventval)

#endif
