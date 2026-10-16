include $(GDK)/makefile.gen

# SGDK pads both builds before this recipe runs. Header fields are outside
# the checksum range; retain SGDK's checksum and describe the final extent.
release debug:
	python3 tools/finalize_rom.py $(OUT)/rom.bin
