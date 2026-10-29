# BPSec Library guide

[BPSec Library (BSL)][bsl] is an external BPSec implementation
that ION can use as an alternative to ION's built-in BPSec.
BSL implements [RFC 9172][rfc9172] / [RFC 9173][rfc9173] security contexts,
uses OpenSSL for cryptography,
and is configured through JSON files rather than `bpsecadmin` commands.

When ION is built with BSL,
it replaces ION's native BPSec policy engine and security processing.

## Build and install

The [BSL Product Guide][bsl-product-guide] provides a thorough explanation
on how to build and install BSL.

!!! note

    ION needs BSL to be built with `BUILD_MOCK_BPA` enabled.

ION's `configure` script uses the `--with-bsl` flag to build ION with BSL;
the script checks for BSL with `pkg-config`.
If BSL is installed in a non-standard prefix (e.g., `/usr/local/`),
adjust the `PKG_CONFIG_PATH` variable
to include the path for `bsl.pc`.

## Configuration

### Runtime setup

BSL uses JSON files for keys and policies. Configure them in your `.bprc` file:

```
m bsl 'ipn:2.0' '/path/to/keys.json' '/path/to/policy.json'
```

**Key file** (`keys.json`):
```json
{
  "keys": [
    {
      "kty": "oct",
      "kid": "myHmacKey",
      "k": "cXdlcnR5dWlvcGFzZGZnaA=="
    }
  ]
}
```

**Policy file** (`policy.json`):
```json
{
  "policyrule_set": [
    {
      "policyrule": {
        "filter": {
          "rule_id": "1",
          "role": "s",
          "src": "ipn:2.*",
          "dest": "ipn:3.*",
          "tgt": 1,
          "loc": "appin",
          "sc_id": 1
        },
        "spec": {
          "sc_id": 1,
          "sc_parms": [
            {"id": "key_name", "value": "myHmacKey"},
            {"id": "sha_variant", "value": "5"},
            {"id": "scope_flags", "value": "7"}
          ]
        }
      }
    }
  ]
}
```

### EID pattern matching and wildcards

BSL policy files support wildcard patterns for matching source and destination EIDs. Understanding how 2-part and 3-part IPN formats interact with wildcards is essential for writing correct security policies.

#### ipn URI scheme

ION supports the 'ipn' scheme URI described in [RFC 9758](https://www.rfc-editor.org/info/rfc9758/)
to specify EIDs.

[TODO: delete this admonition when it's fixed]::

!!! bug

    As of ION 4.2.0,
    parts of ION do not properly validate
    if the textual representation of an ipn URI
    complies with the syntax described in RFC 9758.
    For example,
    `bpadmin` allows adding `ipn:1.03` as an endpoint,
    though such an endpoint does not work properly in ION.

    Stick to valid ipn URIs to avoid things breaking in surprising ways
    (though LocalNode ipn URIs with the character '`!`'
    are not supported).

Either textual representation of ipn EIDs using the Default Allocator
(including or omitting <code><i>allocator-identifier</i>.</code>)
can be used in policies.

#### Wildcard behavior

The `*` wildcard matches any value in its position:

| Pattern | Interpretation | Matches |
|---------|----------------|---------|
| `ipn:2.*` | allocator=0, node=2, service=any | `ipn:0.2.1`, `ipn:0.2.5` |
| `ipn:0.2.*` | allocator=0, node=2, service=any | Same as above |
| `ipn:2.*.*` | allocator=2, node=any, service=any | `ipn:2.1.1`, `ipn:2.3.5` |
| `ipn:0.*.*` | allocator=0, node=any, service=any | All default-allocator EIDs |
| `ipn:*.*.*` | any | All ipn EIDs |

2-part `ipn:2.*` is automatically expanded to 3-part `ipn:0.2.*` — the `*` applies to the service field only. In 3-part format, each `*` matches exactly the field in its position.

## Supported cipher suites

### BIB (Block Integrity Block) - security context 1

- HMAC-SHA256 (variant 5)
- HMAC-SHA384 (variant 6)
- HMAC-SHA512 (variant 7)

### BCB (Block Confidentiality Block) - security context 2

- AES-128-GCM (variant 1)
- AES-256-GCM (variant 3)

## BIB and BCB policy design (RFC 9172 guidelines)

When writing BSL policy files, the relationship between BIB (integrity) and BCB (confidentiality) blocks must follow RFC 9172 rules.

### Same source, same target: Use a single BCB

Per RFC 9172 Section 4.7, if a single security source applies **both** integrity and confidentiality to the **same target block**, it **must** use a single BCB whose security context provides integrity as an additional result — not separate BIB and BCB blocks.

Applying separate BIB and BCB to the same target from the same node creates an ordering problem: the BIB computes its HMAC over plaintext, then the BCB encrypts the payload. A verifier in transit cannot verify the BIB because the payload is ciphertext.

### Different targets: BIB on primary, BCB on payload

When the same node needs both integrity and confidentiality, apply each to a **different** target block:

- **BIB** targets the **primary block** (`tgt: 0`)
- **BCB** targets the **payload block** (`tgt: 1`)

```json
{
  "policyrule_set": [
    {
      "policyrule": {
        "desc": "BIB on primary block",
        "filter": { "role": "s", "src": "ipn:2.*", "dest": "ipn:3.*", "tgt": 0, "sc_id": 1 },
        "spec": {
          "sc_id": 1,
          "sc_parms": [
            {"id": "key_name", "value": "9100"},
            {"id": "sha_variant", "value": "7"},
            {"id": "scope_flags", "value": "0"},
            {"id": "key_wrap", "value": "0"}
          ]
        }
      }
    },
    {
      "policyrule": {
        "desc": "BCB on payload block",
        "filter": { "role": "s", "src": "ipn:2.*", "dest": "ipn:3.*", "tgt": 1, "sc_id": 2 },
        "spec": {
          "sc_id": 2,
          "sc_parms": [
            {"id": "key_name", "value": "9103"},
            {"id": "aes_variant", "value": "1"},
            {"id": "aad_scope", "value": "0"},
            {"id": "key_wrap", "value": "1"}
          ]
        }
      }
    }
  ]
}
```

### Different sources: BIB and BCB on same target is allowed

RFC 9172 permits separate BIB and BCB on the same target when applied by **different nodes** (e.g., Node A adds a BIB, Node B adds a BCB on the same payload). This is valid because the nodes act as independent security sources.

### Security roles

| Role | Code | Location | Description |
|------|------|----------|-------------|
| **Source** | `s` | `appin` | Creates the security block |
| **Verifier** | `v` | `clin` | Checks the block in transit without consuming it |
| **Acceptor** | `a` | `appout` | Processes and removes the block at delivery |

A BIB verifier cannot operate at CLIN when a BCB is present on the same target, because the BIB was computed over plaintext but the payload is still encrypted. When BIB and BCB target different blocks (the recommended approach), this does not apply.

### Multinode policy example

```
Node 2 (source) → Node 3 (relay) → Node 4 (destination)

Node 2: BIB source on primary (tgt 0), BCB source on payload (tgt 1)
Node 3: BCB verifier at CLIN for payload (validates ASB, no decrypt)
Node 4: BCB verifier at CLIN, BIB acceptor at APPOUT, BCB acceptor at APPOUT
```

See `tests/bpsec/bpsec-all-multinode-test.bsl/` for complete working policy files.

## Testing

BSL specific tests are stored in directories ending with `.bsl`.
To perform only these tests,
navigate to the `tests/` directory and use the `runtests` script:

```console
$ ./runtests $(find . -type d -name '*.bsl')
```

!!! info

    `runtests` excludes BSL tests based on heuristics.
    If BSL tests are unexpectedly excluded, check that:

    *   `pkg-config` reports BSL as a private requirement for ION
        (may need to set `PKG_CONFIG_PATH`)

    *   `bpsecadmin` from a previous installation is not in `PATH`

## BSL vs native BPSec

| Feature | BSL | Native BPSec |
|---------|-----|--------------|
| Configuration | JSON files (`m bsl` in bprc) | `bpsecadmin` commands |
| Crypto Library | OpenSSL | Mbed TLS |
| Build Flag | `--with-bsl` | Default (no flag) |

BSL or native BPSec is decided at compile-time,
so switching between them requires rebuilding ION.

## Troubleshooting

### `No package 'bsl' found`

This error occurs at configure-time
when `pkg-config` cannot find BSL's `pkg-config` file:

```console
$ ./configure --with-bsl
[...]
No package 'bsl' found
No package 'bsl' found
No package 'bsl-sample-pp' found
No package 'bsl-default-sc' found

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

Alternatively, you may set the environment variables BSL_CFLAGS
and BSL_LIBS to avoid the need to call pkg-config.
See the pkg-config man page for more details.
```

The hint provided in the error message explains how to fix this.

!!! example

    Inspect the `pc_path` variable to see the default search path
    used by `pkg-config`:

    ```console
    $ pkg-config --variable pc_path pkg-config
    /usr/lib/pkgconfig:/usr/share/pkgconfig
    ```

    Suppose that the BSL shared libraries
    are installed in the `/usr/local/lib/` directory.
    Then `configure` could be run with setting `PKG_CONFIG_PATH`
    to include the BSL `.pc` file in the search path:

    ```console
    $ ./configure PKG_CONFIG_PATH=/usr/local/lib/pkgconfig --with-bsl
    ```

### `cannot open shared object file: libbsl_front.so`

This is a runtime error that occurs when the linker cannot find
the BSL libraries.
If the BSL libraries are installed in a non-standard location,
ensure the linker can locate them
(see the documentation of the system's linker
on how to set search directories).

### `bpadmin` does not recognize `m bsl`

`bpadmin` was built without BSL.
Rebuild with `--with-bsl`.

---

## Appendices

### Appendix A: Key and policy file reference

#### Key file format

```json
{
  "keys": [
    {
      "kty": "oct",
      "kid": "hmac-key-256",
      "k": "cXdlcnR5dWlvcGFzZGZnaGprbHp4Y3Zibm0="
    },
    {
      "kty": "oct",
      "kid": "aes-gcm-key-128",
      "k": "YWJjZGVmZ2hpamtsbW5vcA=="
    }
  ]
}
```

- `kty`: Key type (`oct` for symmetric)
- `kid`: Key identifier (referenced in policy `key_name`)
- `k`: Base64-encoded key material

#### Policy parameters

| Parameter | Values | Description |
|-----------|--------|-------------|
| `role` | `s`, `v`, `a` | Source, verifier, or acceptor |
| `src` / `dest` | EID pattern | Wildcard patterns supported |
| `tgt` | `0`, `1` | Target block (0=primary, 1=payload) |
| `loc` | `appin`, `clin`, `appout` | Processing location |
| `sc_id` | `1`, `2` | Security context (1=BIB, 2=BCB) |

**BIB parameters:** `key_name`, `sha_variant` (5/6/7), `scope_flags`, `key_wrap` (0/1)
**BCB parameters:** `key_name`, `aes_variant` (1/3), `scope_flags`, `key_wrap` (0/1)

#### Validating JSON files

```bash
python3 -m json.tool keys.json
python3 -m json.tool policy.json
```

## Additional resources

-   [BSL GitHub repository][bsl]
-   [BSL Product Guide][bsl-product-guide]
-   [RFC 9172: Bundle Protocol Security (BPSec)][rfc9172]
-   [RFC 9173: Default Security Contexts for BPSec][rfc9173]

[bsl]: https://github.com/NASA-AMMOS/BSL
[bsl-product-guide]: https://nasa-ammos.github.io/BSL-docs/product-guide/html/index.html
[rfc9172]: https://www.rfc-editor.org/info/rfc9172/
[rfc9173]: https://www.rfc-editor.org/info/rfc9173/
