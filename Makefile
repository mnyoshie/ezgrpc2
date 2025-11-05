all :
	$(MAKE) -C src
	$(MAKE) -C examples

clean :
	$(MAKE) -C src clean
	$(MAKE) -C examples clean
