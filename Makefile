BUILDIR = ./build

all: $(BUILDIR)
	+$(MAKE) -C src

$(BUILDIR):
	mkdir -p $(BUILDIR)

clean:
	rm -rf $(BUILDIR)/*


