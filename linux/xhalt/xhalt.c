#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <linux/reboot.h>

int main(int argc, char *argv[]) {
	const char *halt = argc > 1? argv[1] : "0";
	syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, halt);
}
