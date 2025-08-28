# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog and adheres to Semantic Versioning.

## [Unreleased]
- Codegen: Generic, schema-driven encode/decode generation for all messages
- Enum widths: Minimal underlying type selection
- JSON interop: Generated `to_json`/`from_json` stubs per message
- Tools: `mdp_dump` CLI and `pcap_decode` demo
- CI: Cross-platform matrix, sanitizers, fuzz smoke for BOE/ITCH
- Bench: Optional Google Benchmark target
- Visibility: MARKET_* macros and hidden-by-default option
- Zero-copy: Added span-based overloads for encode/decode
- Docs: Schema docs generation and troubleshooting tips
