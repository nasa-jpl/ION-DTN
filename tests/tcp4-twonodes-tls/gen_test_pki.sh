#!/usr/bin/env bash
#
# Generate a self-contained test PKI for TCPCLv4 TLS testing.
#
# Creates a throwaway CA and two node certificates with proper SANs
# including URI-type SANs for RFC 9174 Node ID validation, plus two
# certificates the agents must reject: one from an untrusted CA and one
# that has expired.
#
# Usage: ./gen_test_pki.sh [output_dir]
#   output_dir defaults to ./pki
#
# Environment overrides:
#   NODE2_EID  - Node 2 EID URI for SAN (default: ipn:2.0)
#   NODE3_EID  - Node 3 EID URI for SAN (default: ipn:3.0)
#
# Requires: OpenSSL 1.1.1+
#
# Copyright (c) 2026, California Institute of Technology.
# ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.

set -e

OUTDIR="${1:-./pki}"
NODE2_EID="${NODE2_EID:-ipn:2.0}"
NODE3_EID="${NODE3_EID:-ipn:3.0}"

mkdir -p "$OUTDIR"

echo "Generating test PKI in $OUTDIR ..."

# --- CA ---
openssl ecparam -genkey -name prime256v1 -noout -out "$OUTDIR/ca.key" 2>/dev/null
openssl req -new -x509 -key "$OUTDIR/ca.key" \
    -out "$OUTDIR/ca.crt" -days 3650 \
    -subj "/CN=ION Test CA" 2>/dev/null

# --- Node 2 certificate ---
openssl ecparam -genkey -name prime256v1 -noout -out "$OUTDIR/node2.key" 2>/dev/null
openssl req -new -key "$OUTDIR/node2.key" \
    -out "$OUTDIR/node2.csr" -subj "/CN=node2" 2>/dev/null

openssl x509 -req -in "$OUTDIR/node2.csr" \
    -CA "$OUTDIR/ca.crt" -CAkey "$OUTDIR/ca.key" -CAcreateserial \
    -out "$OUTDIR/node2.crt" -days 3650 \
    -extfile <(printf "subjectAltName=URI:%s,DNS:localhost,IP:127.0.0.1\nbasicConstraints=CA:FALSE" "$NODE2_EID") \
    2>/dev/null

cat "$OUTDIR/node2.crt" "$OUTDIR/ca.crt" > "$OUTDIR/node2-chain.pem"

# --- Node 3 certificate ---
openssl ecparam -genkey -name prime256v1 -noout -out "$OUTDIR/node3.key" 2>/dev/null
openssl req -new -key "$OUTDIR/node3.key" \
    -out "$OUTDIR/node3.csr" -subj "/CN=node3" 2>/dev/null

openssl x509 -req -in "$OUTDIR/node3.csr" \
    -CA "$OUTDIR/ca.crt" -CAkey "$OUTDIR/ca.key" -CAcreateserial \
    -out "$OUTDIR/node3.crt" -days 3650 \
    -extfile <(printf "subjectAltName=URI:%s,DNS:localhost,IP:127.0.0.1\nbasicConstraints=CA:FALSE" "$NODE3_EID") \
    2>/dev/null

cat "$OUTDIR/node3.crt" "$OUTDIR/ca.crt" > "$OUTDIR/node3-chain.pem"

# --- Certificates for the TLS-negative probes ---
openssl ecparam -genkey -name prime256v1 -noout -out "$OUTDIR/rogue-ca.key" 2>/dev/null
openssl req -new -x509 -key "$OUTDIR/rogue-ca.key" \
    -out "$OUTDIR/rogue-ca.crt" -days 3650 \
    -subj "/CN=Rogue Test CA" 2>/dev/null

openssl ecparam -genkey -name prime256v1 -noout -out "$OUTDIR/probe.key" 2>/dev/null
openssl req -new -key "$OUTDIR/probe.key" \
    -out "$OUTDIR/probe.csr" -subj "/CN=node2" 2>/dev/null

# Node 2's identity, signed by a CA the agents don't trust.
openssl x509 -req -in "$OUTDIR/probe.csr" \
    -CA "$OUTDIR/rogue-ca.crt" -CAkey "$OUTDIR/rogue-ca.key" -CAcreateserial \
    -out "$OUTDIR/untrusted.crt" -days 3650 \
    -extfile <(printf "subjectAltName=URI:%s,DNS:localhost,IP:127.0.0.1\nbasicConstraints=CA:FALSE" "$NODE2_EID") \
    2>/dev/null

# Node 2's identity, signed by the test CA, expired yesterday.
openssl x509 -req -in "$OUTDIR/probe.csr" \
    -CA "$OUTDIR/ca.crt" -CAkey "$OUTDIR/ca.key" -CAcreateserial \
    -out "$OUTDIR/expired.crt" -days -1 \
    -extfile <(printf "subjectAltName=URI:%s,DNS:localhost,IP:127.0.0.1\nbasicConstraints=CA:FALSE" "$NODE2_EID") \
    2>/dev/null

# --- Cleanup CSRs and serial ---
rm -f "$OUTDIR"/*.csr "$OUTDIR"/*.srl

echo "PKI generated:"
echo "  CA:     $OUTDIR/ca.crt"
echo "  Node 2: $OUTDIR/node2-chain.pem (SAN: URI:$NODE2_EID)"
echo "  Node 3: $OUTDIR/node3-chain.pem (SAN: URI:$NODE3_EID)"
echo "  Probes: $OUTDIR/untrusted.crt, $OUTDIR/expired.crt (key: probe.key)"
