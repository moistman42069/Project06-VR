1248c60: str d14, [sp, #-0x60]!
1248c64: stp d13, d12, [sp, #8]
1248c68: stp d11, d10, [sp, #0x18]
1248c6c: stp d9, d8, [sp, #0x28]
1248c70: str x30, [sp, #0x38]
1248c74: stp x22, x21, [sp, #0x40]
1248c78: stp x20, x19, [sp, #0x50]
1248c7c: adrp x20, #0x2a07000
1248c80: adrp x21, #0x2840000
1248c84: ldrb w8, [x20, #0xc56]
1248c88: ldr x21, [x21, #0x760]
1248c8c: mov x19, x0
1248c90: tbnz w8, #0, #0x1248cc0
1248c94: adrp x0, #0x283d000
1248c98: ldr x0, [x0, #0x858]
1248c9c: bl #0x10bd824
1248ca0: adrp x0, #0x2840000
1248ca4: ldr x0, [x0, #0x768]
1248ca8: bl #0x10bd824
1248cac: adrp x0, #0x2840000
1248cb0: ldr x0, [x0, #0x760]
1248cb4: bl #0x10bd824
1248cb8: mov w8, #1
1248cbc: strb w8, [x20, #0xc56]
1248cc0: ldr x0, [x21]
1248cc4: mov x1, xzr
1248cc8: bl #0x24e8e98
1248ccc: adrp x21, #0x2a07000
1248cd0: ldrb w8, [x21, #0x8d0]
1248cd4: mov x20, x0
1248cd8: cbnz w8, #0x1248cf0
1248cdc: adrp x0, #0x283d000
1248ce0: ldr x0, [x0, #0x930]
1248ce4: bl #0x10bd824
1248ce8: mov w8, #1
1248cec: strb w8, [x21, #0x8d0]
1248cf0: adrp x8, #0x283d000
1248cf4: ldr x8, [x8, #0x930]
1248cf8: adrp x22, #0x2a07000
1248cfc: adrp x21, #0x283d000
1248d00: ldrb w9, [x22, #0x8d1]
1248d04: ldr x8, [x8]
1248d08: ldr x8, [x8, #0xb8]
1248d0c: ldp s10, s9, [x8]
1248d10: ldr s8, [x8, #8]
1248d14: ldr x21, [x21, #0x858]
1248d18: cbnz w9, #0x1248d30
1248d1c: adrp x0, #0x283d000
1248d20: ldr x0, [x0, #0x9e0]
1248d24: bl #0x10bd824
1248d28: mov w8, #1
1248d2c: strb w8, [x22, #0x8d1]
1248d30: adrp x8, #0x283d000
1248d34: ldr x8, [x8, #0x9e0]
1248d38: ldr x0, [x21]
1248d3c: adrp x21, #0x2840000
1248d40: ldr x8, [x8]
1248d44: ldr w9, [x0, #0xe0]
1248d48: ldr x8, [x8, #0xb8]
1248d4c: ldp s14, s13, [x8]
1248d50: ldp s12, s11, [x8, #8]
1248d54: ldr x21, [x21, #0x768]
1248d58: cbnz w9, #0x1248d60
1248d5c: bl #0x10bd928
1248d60: mov x0, x20
1248d64: mov v0.16b, v10.16b
1248d68: mov v1.16b, v9.16b
1248d6c: mov v2.16b, v8.16b
1248d70: mov v3.16b, v14.16b
1248d74: mov v4.16b, v13.16b
1248d78: mov v5.16b, v12.16b
1248d7c: mov v6.16b, v11.16b
1248d80: mov x1, xzr
1248d84: bl #0x24f0bf0
1248d88: ldr x0, [x21]