# BPSec Configuration Examples

## Updated IPN-URI Format Support (ION 4.1.4-a.2)

Starting with ION 4.1.4-a.2, ION has been updated to support the new IPN URI
scheme defined in [RFC 9758](https://datatracker.ietf.org/doc/html/rfc9758)
as a alpha release feature. The new format is as follows:

```abnf
ipn-uri = "ipn:" [allocator-identifier "."] node-number "." service-number
```

`allocator-identifier`: An unsigned integer identifying the allocation
authority. If the authority is the default (IANA, Allocator ID 0), this
part and the following dot (.) may be omitted for brevity. ION is backward
compatible with IPN URIs that omit the allocator identifier, which is
interpreted as having the default value of 0.

For all examples in this tutorial, the allocator identifier is omitted and
defaults to 0.

New IPN URI support is under alpha testing.

## Overview

The following integrity (BIB) and confidentiality (BCB) security examples demonstrate 3 node networks with security source, security verifier, and security acceptor roles. These hypothetical networks used in the policy examples use ipn node numbers 19 (source), 40 (verifier), and 31(acceptor) and are unidirectional (i.e. from 19 to 31, with 40 serving as a relay) only.

Please keep the following in mind:

* Rule numbers are arbitrary but should never be duplicated at the same node.
* At least one field for either the `src` or `dest` fields must be used (one or both may be used, depending on the desired specificity).
* The wildcard character `*` may be used in place of service numbers only (not node numbers).
* Each of the following (source, verifier, acceptor) examples should exist in its own distinct `.bpsecrc` file (e.g. `<name>.bpsecrc`) and should be initialize during ION startup like this: `bpsecadmin my_node.bpsecrc`

## Integrity (BIB)

### At Security Source

```
# Event Set —--------------------------------------------------------------

a {"event_set" : {"name" : "d_integrity", "desc":"default integrity event set"}}


# Events —-----------------------------------------------------------------

a {"event" : {"es_ref" : "d_integrity", "event_id" : "sop_misconfigured_at_source", "actions" : [{"id" : "remove_sop"},{"id" : "remove_sop_target"} ] }}


# Security Policy —--------------------------------------------------------

a {"policyrule" : {"desc" : "Integrity source rule", "filter" : {"rule_id" : 191, "role" : "s", "src" : "ipn:19.*", "dest" : "ipn:31.*", "tgt" : 1}, "spec" : {"svc" : "bib-integrity", "sc_id" : 1, "sc_parms" : [{"key_name" : "my_hmac_key256" }]}, "es_ref" : "d_integrity" }}
```

### At Security Verifier (optional)

```
# Event Set —--------------------------------------------------------------

a {"event_set" : {"name" : "d_integrity", "desc":"default integrity event set"}}


# Events —-----------------------------------------------------------------

a {"event" : {"es_ref" : "d_integrity", "event_id" : "sop_corrupted_at_verifier", "actions" : [{"id" : "remove_sop"},{"id" : "remove_sop_target"} ] }}


# Security Policy —--------------------------------------------------------

a {"policyrule" : {"desc" : "Integrity verifier rule", "filter" : {"rule_id" : 400, "role" : "v", "src" : "ipn:19.*", "dest" : "ipn:31.*", "tgt" : 1}, "spec" : {"svc" : "bib-integrity", "sc_id" : 1, "sc_parms" : [{"key_name" : "my_hmac_key256" }]}, "es_ref" : "d_integrity" }}
```

### At Security Acceptor

```
# Event Set —--------------------------------------------------------------

a {"event_set" : {"name" : "d_integrity", "desc":"default integrity event set"}}


# Events —-----------------------------------------------------------------

a {"event" : {"es_ref" : "d_integrity", "event_id" : "sop_missing_at_acceptor", "actions" : [{"id" : "remove_sop_target" }]}}

a {"event" : {"es_ref" : "d_integrity", "event_id" : "sop_corrupted_at_acceptor", "actions" : [{"id" : "remove_sop_target" }] }}


# Security Policy —--------------------------------------------------------

a {"policyrule" : {"desc" : "Integrity acceptor rule", "filter" : {"rule_id" : 310, "role" : "a", "src" : "ipn:19.*", "dest" : "ipn:31.*", "tgt" : 1}, "spec" : {"svc" : "bib-integrity", "sc_id" : 1, "sc_parms" : [{"key_name" : "my_hmac_key256" }]}, "es_ref" : "d_integrity" }}
```

## Confidentiality (BCB)

### At Security Source**

```
# Event Set —--------------------------------------------------------------

a {"event_set" : {"name" : "d_bcb_conf", "desc":"default bcb event set"}}


# Events —-----------------------------------------------------------------

a {"event" : {"es_ref" : "d_bcb_conf", "event_id" : "sop_misconfigured_at_source", "actions" : [{"id" : "remove_sop"},{"id" : "remove_sop_target"} ] }}


# Security Policy —--------------------------------------------------------

a {"policyrule" : {"desc" : "Confidentiality source rule", "filter" : {"rule_id" : 192, "role" : "s", "src" : "ipn:19.*", "dest" : "ipn:31.*", "tgt" : 1}, "spec" : {"svc" : "bcb-confidentiality", "sc_id" : 2, "sc_parms" : [{"key_name" : "my_hmac_key256" }, {"aad_scope": "4"}, {"aes_variant":"3"}]}, "es_ref" : "d_bcb_conf" }}
```

### At Security Verifier (optional)

```
# Event Set —--------------------------------------------------------------

a {"event_set" : {"name" : "d_bcb_conf", "desc":"default bcb event set"}}


# Events —-----------------------------------------------------------------

a {"event" : {"es_ref" : "d_bcb_conf", "event_id" : "sop_corrupted_at_verifier", "actions" : [{"id" : "remove_sop"},{"id" : "remove_sop_target"} ] }}


# Security Policy —--------------------------------------------------------

a {"policyrule" : {"desc" : "Confidentiality verifier rule", "filter" : {"rule_id" : 408, "role" : "v", "src" : "ipn:19.*", "dest" : "ipn:31.*", "tgt" : 1}, "spec" : {"svc" : "bcb-confidentiality", "sc_id" : 2, "sc_parms" : [{"key_name" : "my_hmac_key256" }, {"aad_scope": "4"}, {"aes_variant":"3"}]}, "es_ref" : "d_bcb_conf" }}
```

### At Security Acceptor

```
# Event Set —--------------------------------------------------------------

a {"event_set" : {"name" : "d_bcb_conf", "desc":"default bcb event set"}}


# Events —-----------------------------------------------------------------

a {"event" : {"es_ref" : "d_bcb_conf", "event_id" : "sop_corrupted_at_acceptor", "actions" : [{"id" : "remove_sop"},{"id" : "remove_sop_target"} ] }}

a {"event" : {"es_ref" : "d_bcb_conf", "event_id" : "sop_missing_at_acceptor", "actions" : [{"id" : "remove_sop"},{"id" : "remove_sop_target"} ] }}


# Security Policy —--------------------------------------------------------

a {"policyrule" : {"desc" : "Confidentiality acceptor rule", "filter" : {"rule_id" : 311, "role" : "a", "src" : "ipn:19.*", "dest" : "ipn:31.*", "tgt" : 1}, "spec" : {"svc" : "bcb-confidentiality", "sc_id" : 2, "sc_parms" : [{"key_name" : "my_hmac_key256" }, {"aad_scope": "4"}, {"aes_variant":"3"}]}, "es_ref" : "d_bcb_conf" }}
```

## Dependencies

To enable BPSec for ION, the following conditions must be met:

* MBEDTLS 2.28.x is installed with the following:
  * Key Wrapping mode is enabled. See `mbedtls*/include/mbedtls/config.h`, and uncomment the following line:
  ```
  #define MBEDTLS_NIST_KW_C
  ```
  * MBEDTLS has been built with:
  ```
  make SHARED=1
  ```
* ION has been built against MBEDTLS with command as follows (other options maybe included per user's configuration.)
  ```
  ./configure MBED_LIB_PATH=/path-to-mbedtls-library  MBED_INC_PATH=/path-to-mbedtls-header --enable-crypto-mbedtls --enable-bpsec-debugging
  ```
* If bpsec debug logging is not desired/required, the `--enable-bpsec-debugging` option can be omitted.
* A 32 byte symmetric key (HMAC) has been added to the `ionsecadmin` database. Typically this would occur on ION startup but keys can be added/removed at any time. See `man ionsecrc` for synopsis.
