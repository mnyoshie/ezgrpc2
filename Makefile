

help:
	@echo "Available targets:"
	@echo "  all               - Builds src/libezgrpc.{a,so} and examples/*"
	@echo "  clean             - Removes generated *.o, *.so and executables"
	@echo ""
	@echo "  make D=1 all        enables -ggdb3 and server debug logs for development"
	@echo "  make T=1 all        enables server trace logs for development (may provide heavy overheads)"


all :
	$(MAKE) -C src
	$(MAKE) -C examples

clean :
	$(MAKE) -C src clean
	$(MAKE) -C examples clean
