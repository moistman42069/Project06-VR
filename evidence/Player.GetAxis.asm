1fa1d70: str x30, [sp, #-0x30]!
1fa1d74: stp x22, x21, [sp, #0x10]
1fa1d78: stp x20, x19, [sp, #0x20]
1fa1d7c: adrp x22, #0x2a0b000
1fa1d80: adrp x21, #0x2840000
1fa1d84: ldrb w8, [x22, #0x437]
1fa1d88: ldr x21, [x21, #0x768]
1fa1d8c: mov x19, x1
1fa1d90: mov x20, x0
1fa1d94: tbnz w8, #0, #0x1fa1dac
1fa1d98: adrp x0, #0x2840000
1fa1d9c: ldr x0, [x0, #0x768]
1fa1da0: bl #0x10bd824
1fa1da4: mov w8, #1
1fa1da8: strb w8, [x22, #0x437]
1fa1dac: ldr x0, [x21]
1fa1db0: ldr w8, [x0, #0xe0]
1fa1db4: cbnz w8, #0x1fa1dc0
1fa1db8: bl #0x10bd928
1fa1dbc: ldr x0, [x21]
1fa1dc0: ldr x8, [x0, #0xb8]
1fa1dc4: ldr w21, [x20, #0x34]
1fa1dc8: ldr w8, [x8, #0x80]
1fa1dcc: cmp w8, w21
1fa1dd0: b.ne #0x1fa1e08
1fa1dd4: ldr x0, [x20, #0x10]
1fa1dd8: cbz x0, #0x1fa1e34
1fa1ddc: ldr w1, [x20, #0x1c]
1fa1de0: mov w3, #1
1fa1de4: mov x2, x19
1fa1de8: mov x4, xzr
1fa1dec: bl #0x1f538b0
1fa1df0: cbz x0, #0x1fa1e20
1fa1df4: ldp x20, x19, [sp, #0x20]
1fa1df8: ldp x22, x21, [sp, #0x10]
1fa1dfc: mov x1, xzr
1fa1e00: ldr x30, [sp], #0x30
1fa1e04: b #0x1f4dbbc
1fa1e08: ldr w8, [x0, #0xe0]
1fa1e0c: cbnz w8, #0x1fa1e14
1fa1e10: bl #0x10bd928
1fa1e14: mov w0, w21
1fa1e18: mov x1, xzr
1fa1e1c: bl #0x1ff3970
1fa1e20: ldp x20, x19, [sp, #0x20]
1fa1e24: ldp x22, x21, [sp, #0x10]
1fa1e28: fmov s0, wzr
1fa1e2c: ldr x30, [sp], #0x30
1fa1e30: ret 
1fa1e34: bl #0x10bda40