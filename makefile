.PHONY: clean build test

# ============================================================================ #
#  BUILD
# ============================================================================ #
build:
	@mkdir -p build/ && cd build/ && cmake ..

build-test:
	@mkdir -p build/
	@cd build/ && cmake -DCOOM_DEBUG=OFF -DCOOM_ASSERT=ON ..
	@cd build/ && make test_unit


# ============================================================================ #
#  CLEAN
# ============================================================================ #
clean:
	@rm -r -f build/

# ============================================================================ #
#  TEST
# ============================================================================ #
test: | build-test
	@rm -rf *.tpie
	@./build/test/test_unit --reporter=info --colorizer=light
	@rm -rf *.tpie

# ============================================================================ #
#  DOT FILE output for visual debugging
# ============================================================================ #
F =

dot:
	@mkdir -p build/
	@cd build/ && cmake -DCOOM_DEBUG=OFF -DCOOM_ASSERT=OFF ..
	@cd build/ && make coom_dot
	@./build/src/coom/coom_dot ${F}

# ============================================================================ #
#  MAIN for console debugging
# ============================================================================ #
main:
	@mkdir -p build/
	@cd build/ && cmake -DCOOM_DEBUG=ON -DCOOM_ASSERT=ON ..
	@cd build/ && make coom_main
	@rm -rf *.tpie
	@echo "" && echo ""
	@./build/src/coom/coom_main
	@rm -rf *.tpie

# ============================================================================ #
#  EXAMPLES
# ============================================================================ #
N = 8

example-n-queens:
  # Build
	@mkdir -p build/
	@cd build/ && cmake -DCOOM_DEBUG=OFF -DCOOM_ASSERT=OFF ..

	@cd build/ && make n_queens

  # Run
	@echo ""
	@./build/example/n_queens ${N}
	@echo ""
