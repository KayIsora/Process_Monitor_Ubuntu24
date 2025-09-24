TARGETS := pm_kself
all: $(TARGETS)
pm_kself: pm_kself.c
	$(CC) -O2 -Wall $< -o $@
pm_kself_data: pm_kself_data.c
	$(CC) -O2 -Wall -o pm_kself_data pm_kself_data.c
clean:
	rm -f $(TARGETS)
