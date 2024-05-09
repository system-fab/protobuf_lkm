PBTOOLS := $(abspath ./libs/pbtools)
PBTOOLS_LIB := $(abspath ./libs/pbtools/lib)
PY_MOD_ROOT := $(abspath ./protobuf_lkm)
LKM_COMMON := $(abspath ./protobuf_lkm/lkm_common)

all: test

$(PY_MOD_ROOT)/parser.py: $(PBTOOLS)/pbtools/parser.py
	sed 's/pbtools.parse/protobuf_lkm.parse/g' $(PBTOOLS)/pbtools/parser.py > $(PY_MOD_ROOT)/parser.py

$(PY_MOD_ROOT)/errors.py: $(PBTOOLS)/pbtools/errors.py
	cp $(PBTOOLS)/pbtools/errors.py $(PY_MOD_ROOT)/errors.py

$(LKM_COMMON)/generate_pbtools_c.py $(LKM_COMMON)/generate_pbtools_h.py $(LKM_COMMON)/c_source.py:
	bash generate_files.sh

.PHONY: protobuf_lkm
protobuf_lkm: $(PY_MOD_ROOT)/parser.py $(PY_MOD_ROOT)/errors.py $(LKM_COMMON)/generate_pbtools_c.py $(LKM_COMMON)/generate_pbtools_h.py $(LKM_COMMON)/c_source.py

test: venv protobuf_lkm
	export PYTHONPATH=$(shell pwd)
	./venv/bin/python3 -m protobuf_lkm generate_lkm_source -T udp_socket -o test_gen_udp examples/common/address_book/proto/address_book.proto
	./venv/bin/python3 -m protobuf_lkm generate_lkm_source -T netfilter -o test_gen_nf examples/common/address_book/proto/address_book.proto

venv:
	python3 -m venv ./venv
	export PYTHONPATH=$(shell pwd)
	./venv/bin/pip3 install textparser

clean-app:
	rm -f $(PY_MOD_ROOT)/parser.py
	rm -f $(PY_MOD_ROOT)/errors.py
	rm -f $(LKM_COMMON)/generate_pbtools_c.py
	rm -f $(LKM_COMMON)/generate_pbtools_h.py
	rm -f $(LKM_COMMON)/c_source.py

clean-examples:
	rm -rf venv
	rm -rf test_gen_udp
	rm -rf test_gen_nf

	$(MAKE) -C examples/socket_udp/address_book/module clean
	$(MAKE) -C examples/socket_udp/hello_world/module clean

	$(MAKE) -C examples/netfilter/address_book/module clean
	$(MAKE) -C examples/netfilter/floats/module clean
	$(MAKE) -C examples/netfilter/hello_world/module clean

clean: clean-app clean-examples

.PHONY: docs
docs:
	docker build -t dav-11/protobuf_lkm/docs:local -f docs/.docker/Dockerfile .
	docker run --rm --name pbtools-lkm-docs -p 9090:80 dav-11/protobuf_lkm:local