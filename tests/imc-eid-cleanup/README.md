# IMC parser-copy cleanup regression

Runs normal IMC endpoint add/delete commands on the UDP loopback node. After
allocator warmup, each batch must stay within 64 bytes of the baseline allocated working-memory
usage (sampled over three polls to reduce transient daemon scratch), no pending bundles, and no temporary endpoint. Stock integration retains
48 bytes per cycle in the group-zero registration, dispatch and petition paths.

Run through `tests/runtests imc-eid-cleanup`. Requires an IMC-enabled
build; missing optional tools cause a skip. The test starts and stops ION, so use
an isolated test environment as with the other node tests.
