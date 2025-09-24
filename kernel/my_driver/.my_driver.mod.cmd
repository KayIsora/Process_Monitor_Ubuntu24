savedcmd_my_driver.mod := printf '%s\n'   src/pm_core.o src/pm_procfs.o src/pm_cpu.o src/pm_mem.o src/pm_inf.o | awk '!x[$$0]++ { print("./"$$0) }' > my_driver.mod
