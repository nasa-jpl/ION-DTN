# Endpoint information regression

Lists the registered loopback endpoints, queries each exact EID through bpadmin,
and checks that a nonexistent endpoint still reports Unknown endpoint.

Run through `tests/runtests bpadmin-endpoint-info`. The test
starts and stops ION, so use an isolated test environment.
