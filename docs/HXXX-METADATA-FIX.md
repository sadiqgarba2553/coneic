# Preserve timing when converting Annex-B buffers

The first Ubuntu build passed all native UI checks but a stricter H.264 export check found MP4 files with invalid near-zero durations. This was traced to `modules/packetizer/hxxx_nal.c`, in the shared H.264/H.265 Annex-B conversion helper.

When conversion needs to grow or shrink a buffer, the upstream 3.0.24 helper allocates a replacement and copies the payload, but does not copy the original block's properties. PTS, DTS, duration, sample count and flags are lost. In-place conversions preserve them. The fix calls the existing `block_CopyProperties` immediately after allocating the replacement. Payload conversion, allocation ownership, public APIs, configuration options and module registrations are unchanged.

This is a deliberately bounded engine correction, separate from presentation allowances. The source-preservation audit recognises only the exact baseline and corrected SHA-256 pair. Any further change to that file requires review again; it is not placed in the Qt presentation allowlist.

- Baseline SHA-256: `227871d260f0bca99fcdfdf0f2e3b284dce5b621b6d0af6d0d7a3e5febee696f`
- Corrected SHA-256: `3b857fad7d8499c526d6d0bab1a4bcf5fdf9b09be6f8604793b7db1c93b471fa`

The regression probe links to the real native libvlccore allocator. It tests growing multi-NAL conversions, in-place conversions, one-byte and two-byte output lengths, and a single-NAL conversion. It asserts both exact payload bytes and all five metadata fields. With the upstream helper, all payload assertions pass but three allocation-path metadata checks fail. With the correction, all five cases pass. The baseline and corrected reports are retained with the Linux validation evidence.

After building on Linux, reproduce the corrected probe with:

```sh
bash coneic/tests/run_hxxx_metadata.sh
```

The correction is shared source, while the previously published Windows binary remains unchanged. It will receive this correction when rebuilt; the Linux package includes it. This buffer regression does not certify every H.265 stream or export profile.
