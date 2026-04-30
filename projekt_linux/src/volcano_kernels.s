.intel_syntax noprefix

.section .rodata
.align 32

# Scalar constants
vk_zero:              .float 0.0
vk_one:               .float 1.0
vk_eps:               .float 0.0001
vk_inv_temp_s:        .float 0.00111111111
vk_temp_norm_max_s:   .float 1.6
vk_shape_bias:        .float 0.35
vk_shape_scale:       .float 0.65
vk_buoyancy_k:        .float 0.09
vk_wind_bias:         .float 0.4
vk_wind_scale:        .float 0.5
vk_damp_scale:        .float 0.02
vk_damp_min:          .float 0.90
vk_damp_max:          .float 0.998
vk_min_rad2_s:        .float 0.04
vk_vent_rad2_s:       .float 1.2
vk_vent_scale_s:      .float 5.2
vk_life_base_s:       .float 0.14
vk_life_scale_s:      .float 0.10
vk_cool_base_s:       .float 0.55
vk_cool_scale_s:      .float 0.6
vk_cool_rad_s:        .float 0.1

# Vector constants
vk_vec_zero:          .float 0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0
vk_vec_one:           .float 1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0
vk_vec_four:          .float 4.0,4.0,4.0,4.0,4.0,4.0,4.0,4.0
vk_vec_six:           .float 6.0,6.0,6.0,6.0,6.0,6.0,6.0,6.0
vk_vec_eps:           .float 0.000001,0.000001,0.000001,0.000001,0.000001,0.000001,0.000001,0.000001
vk_vec_drop_eps:      .float 0.0001,0.0001,0.0001,0.0001,0.0001,0.0001,0.0001,0.0001
vk_vec_shape_bias:    .float 0.35,0.35,0.35,0.35,0.35,0.35,0.35,0.35
vk_vec_shape_scale:   .float 0.65,0.65,0.65,0.65,0.65,0.65,0.65,0.65
vk_vec_inv_temp:      .float 0.00111111111,0.00111111111,0.00111111111,0.00111111111,0.00111111111,0.00111111111,0.00111111111,0.00111111111
vk_vec_temp_max:      .float 1.6,1.6,1.6,1.6,1.6,1.6,1.6,1.6
vk_vec_min_rad2:      .float 0.04,0.04,0.04,0.04,0.04,0.04,0.04,0.04
vk_vec_vent_rad2:     .float 1.2,1.2,1.2,1.2,1.2,1.2,1.2,1.2
vk_vec_vent_scale:    .float 5.2,5.2,5.2,5.2,5.2,5.2,5.2,5.2
vk_vec_buoy_scale:    .float 0.09,0.09,0.09,0.09,0.09,0.09,0.09,0.09
vk_vec_wind_bias:     .float 0.4,0.4,0.4,0.4,0.4,0.4,0.4,0.4
vk_vec_wind_scale:    .float 0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5
vk_vec_damp_scale:    .float 0.02,0.02,0.02,0.02,0.02,0.02,0.02,0.02
vk_vec_damp_min:      .float 0.90,0.90,0.90,0.90,0.90,0.90,0.90,0.90
vk_vec_damp_max:      .float 0.998,0.998,0.998,0.998,0.998,0.998,0.998,0.998
vk_vec_life_base:     .float 0.14,0.14,0.14,0.14,0.14,0.14,0.14,0.14
vk_vec_life_scale:    .float 0.10,0.10,0.10,0.10,0.10,0.10,0.10,0.10
vk_vec_cool_base:     .float 0.55,0.55,0.55,0.55,0.55,0.55,0.55,0.55
vk_vec_cool_scale:    .float 0.6,0.6,0.6,0.6,0.6,0.6,0.6,0.6
vk_vec_cool_rad:      .float 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1
vk_vec_fluid_min:     .float 650.0,650.0,650.0,650.0,650.0,650.0,650.0,650.0
vk_vec_inv_fluid:     .float 0.00153846154,0.00153846154,0.00153846154,0.00153846154,0.00153846154,0.00153846154,0.00153846154,0.00153846154
vk_vec_k_base:        .float 0.18,0.18,0.18,0.18,0.18,0.18,0.18,0.18
vk_vec_k_scale:       .float 1.22,1.22,1.22,1.22,1.22,1.22,1.22,1.22
vk_vec_max_out:       .float 0.94,0.94,0.94,0.94,0.94,0.94,0.94,0.94
vk_vec_max_temp:      .float 1520.0,1520.0,1520.0,1520.0,1520.0,1520.0,1520.0,1520.0
vk_vec_lava_heat_b:   .float 420.0,420.0,420.0,420.0,420.0,420.0,420.0,420.0
vk_vec_lava_heat_s:   .float 100.0,100.0,100.0,100.0,100.0,100.0,100.0,100.0

.text

# .macro FLOW_FROM_DROP_AVX dst, drop, kval, tmp
# dst  - ymm registr s hodnotou (drop)
# kval - ymm registr s koeficientem k
# tmp  - pomocný ymm registr
.macro FLOW_FROM_DROP_AVX dst, drop, kval, tmp
    vmaxps \dst, \drop, ymm14        # ymm14 = 0.0 (zahodíme záporný spád)
    vsqrtps \tmp, \dst               # odmocnina ze spádu
    vmulps \dst, \dst, \tmp          # drop * sqrt(drop)
    vmulps \dst, \dst, \kval         # k * (drop^1.5)
.endm


# Linux / System V AMD64 notes:
# Integer/pointer args: rdi, rsi, rdx, rcx, r8, r9, then stack
# Float args in scalar/vector registers: xmm0-xmm7 (depending on signature)
# Callee-saved GPRs: rbx, rbp, r12-r15
# Red zone exists on Linux, but for student code prefer explicit stack frame.

# -----------------------------------------------------------------------------
# float volcanoFlowFromDropAsm(float drop, float k);
# drop -> xmm0, k -> xmm1
# -----------------------------------------------------------------------------
.global volcanoFlowFromDropAsm # DONE
volcanoFlowFromDropAsm:
    # TODO: scalar helper used by lava flow

    # xmm0 = drop
    # xmm1 = k

    # pos = max(0, drop)
    vxorps xmm2, xmm2, xmm2
    vmaxss xmm0, xmm0, xmm2     # xmm0 = pos

    # tmp = pos + eps
    movss xmm3, [rip + vk_eps]
    vaddss xmm3, xmm0, xmm3

    # sqrt
    vsqrtss xmm3, xmm3, xmm3

    # shape = a0 + a1 * sqrt(...)
    movss xmm4, [rip + vk_shape_bias]
    vmulss xmm3, xmm3, xmm4

    movss xmm5, [rip + vk_shape_scale]
    vaddss xmm3, xmm3, xmm5

    # result = pos * k * shape
    vmulss xmm0, xmm0, xmm1
    vmulss xmm0, xmm0, xmm3

    ret

# -----------------------------------------------------------------------------
# void volcanoParticleStepAsm(...)
# Pointer args arrive in:
# rdi           =   *posX
# rsi           =   *posY
# rdx           =   *posZ
# rcx           =   *velX
# r8            =   *velY
# r9            =   *velZ
# ymm0          =   dt
# ymm1          =   gravity
# ymm2          =   plumeBuoyancy
# ymm3          =   tempNorm
# ymm4          =   damping
# ymm5          =   windX
# ymm6          =   windZ
# ymm7          =   invR
# ymm8          =   ventPush

# Remaining scalar floats follow System V classification rules.
# -----------------------------------------------------------------------------
.global volcanoParticleStepAsm
volcanoParticleStepAsm:
    # TODO: scalar single-particle step for Linux ABI
    movss xmm9, [rsi] # y_i
    # movss xmm12, xmm9
    movss xmm10, [rip + vk_zero]
    maxss xmm9, xmm10 # max (y_i, 0)

    movss xmm10, [rip + vk_buoyancy_k]
    mulss xmm9, xmm10
    movss xmm10, [rip + vk_one]
    addss xmm9, xmm10 # 1 + k_b * max(y_i, 0)

    movss xmm10, xmm2
    mulss xmm10, xmm3 # B * T_{norm}

    divss xmm10, xmm9 # B * T_{norm} / (1 + k_b * max(y_i, 0))

    addss xmm9, xmm1 #                                                                          a_y

    # axis x and z
    movss xmm10, xmm3
    mulss xmm10, [rip + vk_vent_scale_s] # K_{vent} * T_{norm}

    movss xmm11, [rdi] # x
    movss xmm14, xmm11 # preserve x
    mulss xmm11, xmm11 # x^2

    movss xmm13, [rdx] # z_i
    movss xmm15, xmm13 # preserve z
    mulss xmm13, xmm13 # z^2

    addss xmm11, xmm13 # x^2 + z^2 = r^2

    movss xmm12, [rip + vk_vent_rad2_s] # R_{vent}^2
    subss xmm12, xmm11 # R_{vent}^2 - r^2

    movss xmm11, [rip + vk_one]
    maxss xmm11, xmm12 # max(0, R_{vent}^2 - r^2)

    mulss xmm10, xmm11 # K_{vent} * T_{norm} * max(0, R_{vent}^2 - r^2) =                       a_{vent}


    mulss xmm10, xmm7 # a_{vent} * invR


    movss xmm11, xmm3 # T_{norm}
    mulss xmm11, [rip + vk_wind_scale] # c_{w1} * T_{norm}
    addss xmm11, [rip + vk_wind_bias] # c_{w0} + c_{w1} * T_{norm}

    movss xmm12, xmm11

    mulss xmm11, xmm5 # windX * (c_{w0} + c_{w1} * T_{norm})

    mulss xmm14, xmm10 # x_i * a_{vent} * invR
    addss xmm11, xmm14 # windX * (c_{w0} + c_{w1} * T_{norm}) + x_i * a_{vent} * invR =         a_x

    mulss xmm15, xmm10 # z_i * a_{vent} * invR
    mulss xmm12, xmm6 # windZ * (c_{w0} + c_{w1} * T_{norm})
    addss xmm12, xmm15 # windZ * (c_{w0} + c_{w1} * T_{norm}) + z_i * a_{vent} * invR =         a_z
    


    mulss xmm9, xmm0 # a_y * dt
    mulss xmm11, xmm0 # a_x * dt
    mulss xmm12, xmm0 # a_z * dt

    addss xmm9, [r8] # v_y
    addss xmm11, [rcx] # v_x
    addss xmm12, [r9] # v_z


    movss xmm13, [rip + vk_one] # assume d_0 = 1.0
    movss xmm14, [rip + vk_damp_scale] # assume d_1 = vk_damp_scale

    mulss xmm14, xmm3 # d_1 * T_{norm}
    subss xmm13, xmm14 # d_0 - d_1 * T_{norm}

    movss xmm14, [rip + vk_damp_min]
    maxss xmm13, xmm14 # min(d_0 - d_1 * T_{norm}, d_{min})

    movss xmm14, [rip + vk_damp_max]
    minss xmm13, xmm14 # clamp(d_0 - d_1 * T_{norm}, d_{max}, d_{min}) =                        d_{local}


    mulss xmm9, xmm13  # v_y = v_y * d_{local}
    mulss xmm11, xmm13 # v_x = v_x * d_{local}
    mulss xmm12, xmm13 # v_z = v_z * d_{local}


    movss [r8], xmm9   # store v_y
    movss [rcx], xmm11 # store v_x
    movss [r9], xmm12  # store v_z


    mulss xmm9, xmm0  # v_y * dt
    mulss xmm11, xmm0 # v_x * dt
    mulss xmm12, xmm0 # v_z * dt

    addss xmm9, [rsi] # p_y += v_y * dt
    movss [rsi], xmm9

    addss xmm11, [rdi] # p_x += v_x * dt
    movss [rdi], xmm11

    addss xmm12, [rdx] # p_z += v_z * dt
    movss [rdx], xmm12

    ret

# -----------------------------------------------------------------------------
# void volcanoParticleStepBatchAsm(...)
# -----------------------------------------------------------------------------
.global volcanoParticleStepBatchAsm
# rdi           =   *posX
# rsi           =   *posY
# rdx           =   *posZ
# rcx           =   *velX
# r8            =   *velY
# r9            =   *velZ
# rbp + 0x10    =   *life
# rbp + 0x18    =   *temperature
# rbp + 0x20    =   particleCount
# rbp + 0x28    =   &{
#     + 0                dt,
#     + 4                gravity,
#     + 8                plumeBuoyancy,
#     + 12               ambientTemperature,
#     + 16               damping,
#     + 20               windX,
#     + 24               windZ,
#     + 28               particleCooling
#                    };
volcanoParticleStepBatchAsm:
    # TODO:
    # 1. split count into simdCount and scalar tail
    # 2. call volcanoParticleStepSIMDAsm for full 8-wide blocks
    # 3. finish remaining particles with scalar path
    push    rbp
    mov     rbp, rsp

    xor r10, r10
    mov r13, rdx # backup
    mov r11, [rbp + 0x20] # particleCount
    mov rax, r11
    xor rdx, rdx
    mov r14, 8
    div r14
    sub r11, rdx # remainder
    mov r15, rdx # preserve remainder
    mov rdx, r13 # restore

    # r10 = counter (0, 8, 16, 24, 32...)
    # r11 = loop limit
    # r12 = struct *
    mov r12, [rbp + 0x28]
.loop:
    mov r14, r10
    shl r14, 2 # incrementing by 8 (=2^3) indexes, multiply by 4 (2^2) (sizeof(float))

    # increment pointers
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9

    add rdi, r14
    add rsi, r14
    add rdx, r14
    add rcx, r14
    add r8, r14
    add r9, r14

    # push args last to first
    push r12 # const

    push qword ptr [rbp + 0x20] # const

    mov r13, [rbp + 0x18]
    add r13, r14 # processed another 8, add to offset
    push r13

    mov r13, [rbp + 0x10]
    add r13, r14 # processed another 8, add to offset
    push r13

    # call volcanoParticleStepSIMDAsm
    call volcanoParticleStepSIMDAsm
    # cleanup
    add rsp, 32 # 4 8-byte args
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi



    add r10, 8
    # ab for unsigned
    cmp r11, r10
    ja .loop



    # process tail
    # incremented, loop exited, but the tail is not processed yet
    shl r10, 2 # r10 is index; multiply by sizeof(float) = 4
    # r15 = remainder
    xor r11, r11 # counter

    # increment pointers by what's been processed
    add rdi, r10
    add rsi, r10
    add rdx, r10
    add rcx, r10
    add r8, r10
    add r9, r10


    mov r12, [rbp + 0x18] # *temperature
    mov r13, [rbp + 0x28] # struct

.tail_loop:
    cmp r15, r11
    jna .tail_loop_end
# void volcanoParticleStepAsm(...)
# Pointer args arrive in:
# rdi           =   *posX
# rsi           =   *posY
# rdx           =   *posZ
# rcx           =   *velX
# r8            =   *velY
# r9            =   *velZ
# ymm0          =   dt
# ymm1          =   gravity
# ymm2          =   plumeBuoyancy
# ymm3          =   tempNorm
# ymm4          =   damping
# ymm5          =   windX
# ymm6          =   windZ
# ymm7          =   invR
# ymm8          =   ventPush
    # calculate invR
    movss xmm0, [rdi] # x
    mulss xmm0, xmm0 # x^2
    movss xmm1, [rdx] # z
    mulss xmm1, xmm1 # z^2

    addss xmm0, xmm1 # r^2 = x^2 + z^2
    movss xmm2, xmm1 # preserve r^2
    movss xmm1, [rip + vk_min_rad2_s]

    maxss xmm0, xmm1 # max(r^2 = x^2 + z^2, \eps_r)
    rsqrtss xmm0, xmm0 # 1/sqrt(max(r^2 = x^2 + z^2, \eps_r))
    movss xmm7, xmm0

    # calculate tempNorm
    movss xmm0, [r12] # temperature
    movss xmm1, [r13 + 12] # ambient temperature

    subss xmm0, xmm1 # T_i - t_{amb}
    movss xmm1, [rip + vk_inv_temp_s]
    mulss xmm0, xmm1 # (T_i - t_{amb}) / T_{span} # inverse
    movss xmm1, [rip + vk_zero]
    maxss xmm0, xmm1
    movss xmm1, [rip + vk_temp_norm_max_s]
    minss xmm0, xmm1 # clamp((T_i - t_{amb}) / T_{span}, 0, T_{normMax})

    movss xmm3, xmm0

    # calculate ventPush
    # const float ventPush = max0(1.2f - radius2) * tempNorm * 5.2f;
    movss xmm0, [rip + vk_vent_rad2_s]
    subss xmm0, xmm2 # 1.2f - r^2
    movss xmm1, [rip + vk_zero]
    maxss xmm0, xmm1 # max0(1.2f - r^2)

    mulss xmm0, xmm3 # max0(1.2f - r^2) * tempNorm
    movss xmm1, [rip + vk_vent_scale_s]
    mulss xmm0, xmm1 # max0(1.2f - r^2) * tempNorm * 5.2f

    movss xmm8, xmm0

    # pass dt
    movss xmm0, [r13 + 0]
    # pass gravity
    movss xmm1, [r13 + 4]
    # pass plumeBuoyancy
    movss xmm2, [r13 + 8]
    # pass tempNorm (calculated)
    # pass damping
    movss xmm4, [r13 + 16]
    # pass windX
    movss xmm5, [r13 + 20]
    # pass windZ
    movss xmm6, [r13 + 24]
    # pass invR (calculated)
    # pass ventPush (calculated)
    call volcanoParticleStepAsm

    add rdi, 4 # sizeof(float)
    add rsi, 4 # sizeof(float)
    add rdx, 4 # sizeof(float)
    add rcx, 4 # sizeof(float)
    add r8, 4 # sizeof(float)
    add r9, 4 # sizeof(float)
    add r12, 4 # sizeof(float)

    
    inc r11
    jmp .tail_loop
.tail_loop_end:

    pop rbp
    ret

# -----------------------------------------------------------------------------
# void volcanoParticleStepSIMDAsm(...)
# -----------------------------------------------------------------------------
.global volcanoParticleStepSIMDAsm
# rdi           =   *posX
# rsi           =   *posY
# rdx           =   *posZ
# rcx           =   *velX
# r8            =   *velY
# r9            =   *velZ
# rbp + 0x10    =   *life
# rbp + 0x18    =   *temperature
# rbp + 0x20    =   particleCount
# rbp + 0x28    =   &{
#     + 0                dt,
#     + 4                gravity,
#     + 8                plumeBuoyancy,
#     + 12               ambientTemperature,
#     + 16               damping,
#     + 20               windX,
#     + 24               windZ,
#     + 28               particleCooling
#                    };
volcanoParticleStepSIMDAsm:
    # TODO:
    # - load 8 particles (SoA)
    # - compute tempNorm, radius2, invR, ventPush
    # - update velX/velY/velZ
    # - apply damping
    # - update posX/posY/posZ
    # - update life and temp

    push    rbp
    mov     rbp, rsp

    vmovups ymm0, [rdi] # posX
    vmovups ymm1, [rdx] # posZ

    # preserve x, z
    vmovups ymm9, ymm0
    vmovups ymm10, ymm1
    
    vmulps ymm0, ymm0, ymm0 # x^2
    vmulps ymm1, ymm1, ymm1 # z^2

    vaddps ymm0, ymm0, ymm1 # x^2 + z^2                                             
    vmovups ymm11, ymm0 #                                                           = r^2

    vmovups ymm1, [rip + vk_vec_eps]
    vmaxps ymm0, ymm1, ymm0 # max(r^2, \epsilon_r)

    vrsqrtps ymm0, ymm0 # 1/sqrt(max(r^2, \epslon_r))                               = invR

    mov r13, [rbp + 0x28] # struct dereference
    mov r14, [rbp + 0x18] # *temperature dereference

    # ambient temperature
    vbroadcastss ymm1, [r13 + 12] # ambient temperature
    vmovups ymm2, [r14] # temperatures
    vsubps ymm1, ymm2, ymm1 # T_i - T_{amb}
    vmovups ymm2, [rip + vk_vec_inv_temp]
    vmulps ymm1, ymm1, ymm2 # (T_i - T_{amb}) / T_{span} # inverse
    vmovups ymm2, [rip + vk_vec_zero]
    vmaxps ymm1, ymm1, ymm2

    vmovups ymm2, [rip + vk_vec_temp_max]
    vminps ymm1, ymm1, ymm2 # clamp((T_i - T_{amb}) / T_{span}, 0, T_{normMax})     = T_{norm}



    vmovups ymm3, [rsi] # y
    vmovups ymm4, [rip + vk_vec_zero]
    vmaxps ymm3, ymm3, ymm4 # max(y_i, 0)

    vmovups ymm4, [rip + vk_vec_buoy_scale]
    vmulps ymm3, ymm3, ymm4 # k_b * max(y_i, 0)
    vmovups ymm4, [rip + vk_vec_one]
    vaddps ymm3, ymm3, ymm4 # 1 + k_b * max(y_i, 0)

    vbroadcastss ymm4, [r13 + 8] # B
    vmulps ymm4, ymm4, ymm1 # B * T_{norm}

    vdivps ymm3, ymm4, ymm3 # B * T_{norm} / (1 + k_b * max(y_i, 0))

    vbroadcastss ymm4, [r13 + 4] # g
    vaddps ymm3, ymm3, ymm4 # g + B * T_{norm} / (1 + k_b * max(y_i, 0))            = a_y
    

    vbroadcastss ymm4, [rip + vk_vec_vent_rad2]
    vsubps ymm5, ymm4, ymm11 # R^2 - r^2
    vmovups ymm4, [rip + vk_vec_zero]

    vmaxps ymm5, ymm5, ymm4 # max(0, R^2 - r^2)
    vmovups ymm4, [rip + vk_vec_vent_scale]
    vmulps ymm4, ymm4, ymm1 # K_{vent} * T_{norm}

    vmulps ymm4, ymm4, ymm5 # max(0, R^2 - r^2) * K_{vent} * T_{norm}               = a_{vent}


    vmulps ymm4, ymm4, ymm0 # a_{vent} * invR
    vmulps ymm9, ymm9, ymm4 # x_i * a_{vent} * invR
    vmulps ymm10, ymm10, ymm4 # z_i * a_{vent} * invR


    vmovups ymm5, [rip + vk_vec_wind_scale]
    vmulps ymm5, ymm5, ymm1 # T_{norm} * c_{w1}
    vmovups ymm6, [rip + vk_vec_wind_bias]
    vaddps ymm5, ymm5, ymm6 # T_{norm} * c_{w1} + c_{w2}

    vbroadcastss ymm6, [r13 + 20] # windX
    vmulps ymm6, ymm5, ymm6 # windX * (T_{norm} * c_{w1} + c_{w2})

    vaddps ymm6, ymm6, ymm9 # windX * (T_{norm} * c_{w1} + c_{w2}) + x_i * a_{vent} * invR = a_x

    vbroadcastss ymm7, [r13 + 24] # windZ
    vmulps ymm5, ymm5, ymm7 # windZ * (T_{norm} * c_{w1} + c_{w2})

    vaddps ymm5, ymm5, ymm10 # windZ * (T_{norm} * c_{w1} + c_{w2}) + z_i * a_{vent} * invR = a_z
    
    vbroadcastss ymm12, [r13 + 0] # dt
    vmovups ymm8, ymm12
    vmulps ymm3, ymm3, ymm12 # a_y * dt
    vmulps ymm6, ymm6, ymm12 # a_x * dt
    vmulps ymm5, ymm5, ymm12 # a_z * dt

    vmovups ymm7, [r8] # velY
    vaddps ymm3, ymm7, ymm3 # v_y += a_y * dt

    vmovups ymm7, [rcx] # velX
    vaddps ymm6, ymm7, ymm6 # v_x += a_x * dt

    vmovups ymm7, [r9] # velZ
    vaddps ymm5, ymm7, ymm5 # v_z += a_z * dt


    vmovups ymm7, [rip + vk_vec_damp_scale] # assume
    vmulps ymm7, ymm7, ymm1 # d_1 * T_{norm}
    vmovups ymm8, [rip + vk_vec_one] # assume
    vsubps ymm7, ymm8, ymm7 # d_0 - d_1 * T_{norm}

    vmovups ymm8, [rip + vk_vec_damp_min]
    vmaxps ymm7, ymm7, ymm8
    vmovups ymm8, [rip + vk_vec_damp_max]
    vminps ymm7, ymm7, ymm8 # clamp(d_0 - d_1 * T_{norm}, d_{min}, d_{max})         = d_{local}

    vmulps ymm3, ymm3, ymm7 # v_y *= d_{local}
    vmulps ymm6, ymm6, ymm7 # v_x *= d_{local}
    vmulps ymm5, ymm5, ymm7 # v_z *= d_{local}

    vmovups [r8], ymm3
    vmovups [rcx], ymm6
    vmovups [r9], ymm5

    vmulps ymm3, ymm3, ymm12 # v_y * dt
    vmulps ymm6, ymm6, ymm12 # v_x * dt
    vmulps ymm5, ymm5, ymm12 # v_z * dt


    vmovups ymm7, [rsi] # p_y
    vaddps ymm7, ymm7, ymm3
    vmovups [rsi], ymm7

    vmovups ymm7, [rdi] # p_x
    vaddps ymm7, ymm7, ymm6
    vmovups [rdi], ymm7

    vmovups ymm7, [rdx] # p_z
    vaddps ymm7, ymm7, ymm5
    vmovups [rdx], ymm7



    vmovups ymm7, [rip + vk_vec_life_scale]
    vmulps ymm7, ymm7, ymm1 # c_{l1} * T_{norm}

    vmovups ymm8, [rip + vk_vec_life_base]
    vaddps ymm7, ymm7, ymm8 # c_{l0} + c_{l1} * T_{norm}

    # vbroadcastss ymm8, [r13 + 0] # dt
    vmulps ymm7, ymm7, ymm12 # dt * (c_{l0} + c_{l1} * T_{norm})

    mov r14, [rbp + 0x10]
    vmovups ymm9, [r14] # life_i

    vsubps ymm9, ymm9, ymm7 # life_i - dt * (c_{l0} + c_{l1} * T_{norm})
    vmovups [r14], ymm9


    vmovups ymm7, ymm2 # r^2
    vmovups ymm9, [rip + vk_vec_cool_rad]
    vaddps ymm7, ymm7, ymm9 # r^2 + \eps_t

    vsqrtps ymm7, ymm7 # sqrt(r^2 + \eps_t)

    vmovups ymm9, [rip + vk_vec_cool_scale]
    vmulps ymm7, ymm7, ymm9 # c_{t1} * sqrt(r^2 + \eps_t)

    vmovups ymm9, [rip + vk_vec_cool_base] # c_{t0}

    vaddps ymm7, ymm7, ymm9 # c_{t0} + c_{t1} * sqrt(r^2 + \eps_t)

    vmulps ymm7, ymm7, ymm12 # dt * (c_{t0} + c_{t1} * sqrt(r^2 + \eps_t))

    vbroadcastss ymm8, [r13 + 28] # k_{cool}

    vmulps ymm7, ymm7, ymm8 # k_{cool} * dt * (c_{t0} + c_{t1} * sqrt(r^2 + \eps_t))

    mov r14, [rbp + 0x18] # *temperature dereference

    vmovups ymm8, [r14] # T_i

    vsubps ymm7, ymm8, ymm7 # T_i - k_{cool} * dt * (c_{t0} + c_{t1} * sqrt(r^2 + \eps_t))

    vmovups [r14], ymm7 # store T_i

    pop rbp
    ret

# -----------------------------------------------------------------------------
# void volcanoUpdateLavaFluxSIMDAsm(...)
# -----------------------------------------------------------------------------
.global volcanoUpdateLavaFluxSIMDAsm
# rdi           =   lavaHeightNext.data()
# rsi           =   lavaHeight.data()
# rdx           =   terrainHeight.data()
# rcx           =   temperature.data()
# r8            =   width
# r9            =   height
# ymm0          =   viscDt
.global volcanoUpdateLavaFluxSIMDAsm
volcanoUpdateLavaFluxSIMDAsm:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    # rdi=next, rsi=lava, rdx=terrain, rcx=temp, r8=width, r9=height, xmm0=viscDt
    mov r11, rdx        
    mov r14, rcx        
    
    mov r12, r8
    sub r12, 2
    shr r12, 3
    shl r12, 3
    add r12, 1

    # Načtení konstant
    vbroadcastss ymm15, xmm0                 # ymm15 = viscDt
    vmovups ymm14, [rip + vk_vec_zero]       
    vmovups ymm13, [rip + vk_vec_one]        
    vmovups ymm12, [rip + vk_vec_inv_fluid]  
    vmovups ymm11, [rip + vk_vec_fluid_min]  

    mov r15, r8
    shl r15, 2          # offset řádku

    mov r10, 1          # y = 1
    dec r9              # height - 1

.flux_loop_rows:
    mov rax, r10
    imul rax, r8
    shl rax, 2          
    
    mov r13, 1          # x = 1

    .flux_loop_inner:
        lea rbx, [rax + r13 * 4]

        # 1. Výpočet k_C (koeficient středu)
        vmovups ymm0, [r14 + rbx]            
        vsubps  ymm0, ymm0, ymm11
        vmulps  ymm0, ymm0, ymm12
        vmaxps  ymm0, ymm0, ymm14
        vminps  ymm0, ymm0, ymm13
        vmulps  ymm1, ymm0, [rip + vk_vec_k_scale]
        vaddps  ymm1, ymm1, [rip + vk_vec_k_base]
        vmulps  ymm1, ymm1, ymm15            # ymm1 = k_C

        # 2. Výšky hladin H = T + L
        vmovups ymm2, [r11 + rbx]            
        vaddps  ymm2, ymm2, [rsi + rbx]      # ymm2 = H_C

        # --- OUT FLOW (Odtok ze středu ven) ---
        # Left
        vmovups ymm3, [r11 + rbx - 4]
        vaddps  ymm3, ymm3, [rsi + rbx - 4]
        vsubps  ymm4, ymm2, ymm3             # dropL = HC - HL
        FLOW_FROM_DROP_AVX ymm4, ymm4, ymm1, ymm0
        vmovups ymm10, ymm4                  # outSum = outL

        # Right
        vmovups ymm3, [r11 + rbx + 4]
        vaddps  ymm3, ymm3, [rsi + rbx + 4]
        vsubps  ymm4, ymm2, ymm3             # dropR
        FLOW_FROM_DROP_AVX ymm4, ymm4, ymm1, ymm0
        vaddps  ymm10, ymm10, ymm4

        # Up
        mov rbp, rbx
        sub rbp, r15
        vmovups ymm3, [r11 + rbp]
        vaddps  ymm3, ymm3, [rsi + rbp]
        vsubps  ymm4, ymm2, ymm3             # dropU
        FLOW_FROM_DROP_AVX ymm4, ymm4, ymm1, ymm0
        vaddps  ymm10, ymm10, ymm4

        # Down
        mov rbp, rbx
        add rbp, r15
        vmovups ymm3, [r11 + rbp]
        vaddps  ymm3, ymm3, [rsi + rbp]
        vsubps  ymm4, ymm2, ymm3             # dropD
        FLOW_FROM_DROP_AVX ymm4, ymm4, ymm1, ymm0
        vaddps  ymm10, ymm10, ymm4           # ymm10 = outSum (raw)

        # --- SCALING (Ochrana proti "vysátí" buňky do záporu) ---
        vmovups ymm0, [rsi + rbx]            # ymm0 = lava_C
        vmulps  ymm3, ymm0, [rip + vk_vec_max_out] # ymm3 = limit (např. 94% objemu)
        
        vcmpps  ymm4, ymm10, ymm3, 0x0E      # outSum > limit?
        vmovups ymm5, [rip + vk_vec_eps]
        vdivps  ymm6, ymm3, ymm10            # scale = limit / outSum
        vblendvps ymm6, ymm13, ymm6, ymm4    # if (outSum > limit) scale else 1.0
        
        vmulps  ymm10, ymm10, ymm6           # ymm10 = outSum (scaled)

        # --- IN FLOW (Přítok do středu od sousedů) ---
        vxorps  ymm9, ymm9, ymm9             # ymm9 = inSum
        # Pro inFlow potřebujeme k_sousedů. Zde je největší režie.
        
        # In from Left (potřebuje k_L)
        vmovups ymm4, [r14 + rbx - 4]
        vsubps  ymm4, ymm4, ymm11
        vmulps  ymm4, ymm4, ymm12
        vmaxps  ymm4, ymm4, ymm14
        vminps  ymm4, ymm4, ymm13
        vmulps  ymm4, ymm4, [rip + vk_vec_k_scale]
        vaddps  ymm4, ymm4, [rip + vk_vec_k_base]
        vmulps  ymm4, ymm4, ymm15            # ymm4 = k_L
        vmovups ymm3, [r11 + rbx - 4]
        vaddps  ymm3, ymm3, [rsi + rbx - 4]
        vsubps  ymm3, ymm3, ymm2             # HL - HC
        FLOW_FROM_DROP_AVX ymm3, ymm3, ymm4, ymm5
        vaddps  ymm9, ymm9, ymm3

        # ... (Zde by se musely dopočítat inR, inU, inD) ...
        # Poznámka: Pokud inFlow vynecháš nebo spočítáš blbě, láva teče jen jedním směrem!

        # --- FINAL STORE ---
        vmovups ymm0, [rsi + rbx]
        vaddps  ymm0, ymm0, ymm9
        vsubps  ymm0, ymm0, ymm10
        vmaxps  ymm0, ymm0, ymm14
        vminps  ymm0, ymm0, [rip + vk_vec_six]
        vmovups [rdi + rbx], ymm0

        add r13, 8
        cmp r13, r12
        jb .flux_loop_inner

    inc r10
    cmp r10, r9
    jb .flux_loop_rows

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

# -----------------------------------------------------------------------------
# void volcanoDiffuseHeatSIMDAsm(...)
# -----------------------------------------------------------------------------
.global volcanoDiffuseHeatSIMDAsm
# rdi           =   temperatureNext.data()
# rsi           =   temperature.data()
# rdx           =   lavaHeight.data()
# rcx           =   width
# r8            =   height
# r9            =   
# ymm0          =   alphaDt
# ymm1          =   dt
# ymm2          =   lavaCooling
# ymm3          =   ambientTemperature
volcanoDiffuseHeatSIMDAsm:
    # TODO:
    # - iterate interior cells by rows
    # - compute laplacian
    # - add lava heating, subtract cooling
    # - clamp to [ambientTemperature, 1520]


    mov r10, 1 # y
    dec r8 # height - 1
    mov r11, rdx # preserve terrainHeight.data()

    # const int scalarStartX = 1 + ((width - 2) / 8) * 8;
    mov r12, rcx
    sub r12, 2 # - 2
    shr r12, 3 # / 8
    shl r12, 3 # * 8
    add r12, 1 # + 1

    # viscDt
    vbroadcastss ymm15, xmm3 # broadcast ambientTemperature
    vbroadcastss ymm14, xmm2 # broadcast lavaCooling
    vbroadcastss ymm13, xmm1 # broadcast dt
    vbroadcastss ymm12, xmm0 # broadcast alphaDt
    vdivps ymm12, ymm12, ymm13 # alphaDt / dt = diffusionAlpha

    .diffuse_loop:
    # rax =  # row = y * width
    mov rax, r10 
    mul rcx
    # rdx ignored

    mov rdx, r11

    mov r13, 1 # x
        .diffuse_loop_inner:
        # const std::size_t id = row + static_cast<std::size_t>(x);
        mov rbx, rax
        add rbx, r13 # rbx = id

        # cell
        lea r14, [rbx * 4]
        vmovups ymm0, [rsi + r14 + 0] # temperature_C
        vmovups ymm1, [rsi + r14 - 4] # temperature_L
        vmovups ymm2, [rsi + r14 + 4] # temperature_R
        # lea r9, [r14 - rcx * 4]
        mov r9, r14
        mov r15, rcx
        shl r15, 2 # * 4
        sub r9, r15

        vmovups ymm3, [rsi + r9] # temperature_U
        lea r9, [r14 + rcx * 4]
        vmovups ymm4, [rsi + r9] # temperature_D

        # lap
        vaddps ymm5, ymm1, ymm2
        vaddps ymm6, ymm3, ymm4
        vaddps ymm5, ymm5, ymm6 # t_L + t_R + t_U + t_D

        vmovups ymm6, [rip + vk_vec_four]
        vmulps ymm6, ymm0, ymm6 # 4.0 * t_C

        vsubps ymm5, ymm5, ymm6 # t_L + t_R + t_U + t_D + 4.0 * t_C

        # fluid
        vmovups ymm6, ymm0
        vmovups ymm7, [rip + vk_vec_fluid_min]
        vsubps ymm6, ymm6, ymm7 # t_C - 650.0
        vdivps ymm6, ymm6, ymm7 # (t_C - 650.0) / 650.0
        vmovups ymm7, [rip + vk_vec_zero]
        vmaxps ymm6, ymm6, ymm7
        vmovups ymm7, [rip + vk_vec_one]
        vminps ymm6, ymm6, ymm7 # fluid = clamp((t_C - 650.0) / 650.0, 0.0, 1.0)

        # lavaHeat
        vmovups ymm7, [rdx + r14 + 0] # lavaHeight[id]

        vmovups ymm9, [rip + vk_vec_lava_heat_s]
        vmulps ymm8, ymm9, ymm6 # 100.0 * fluid
        vmovups ymm9, [rip + vk_vec_lava_heat_b]
        vaddps ymm8, ymm9, ymm8 # 420.0 + 100.0 * fluid

        vmulps ymm7, ymm7, ymm8 # lavaHeight[id] * (420.0 + 100.0 * fluid)

        # cooling
        vmovups ymm8, ymm0 # t_C
        vsubps ymm8, ymm8, ymm15 # t_C - ambientTemperature
        vmulps ymm8, ymm8, ymm14 # lavaCooling * (t_C - ambientTemperature)

        # next
        vmulps ymm11, ymm12, ymm5 # diffusionAlpha * lap
        vaddps ymm11, ymm11, ymm7 # diffusionAlpha * lap + lavaHeat
        vsubps ymm11, ymm11, ymm8 # diffusionAlpha * lap + lavaHeat - cooling
        vmulps ymm11, ymm11, ymm13 # dt * (diffusionAlpha * lap + lavaHeat - cooling)
        vaddps ymm11, ymm11, ymm0 # c + dt * (diffusionAlpha * lap + lavaHeat - cooling)

        # std::isfinite(next) ? std::clamp(next, ambientTemperature, 1520.0f) : ambientTemperature;
        # need to preserve only next and ambientTemperature, so ymm0-ymm11 and ymm13-ymm14 free
        
        # clamp(next, ambientTemperature, 1520.0f)
        
        vmovups ymm0, ymm11
        vmaxps ymm0, ymm0, ymm15
        vmovups ymm1, [rip + vk_vec_max_temp]
        vminps ymm0, ymm0, ymm1 # clamp(next, ambientTemperature, 1520.0f)


        # isfinite(next); self comparison
        vcmpps ymm1, ymm11, ymm11, 0x3 # VCMPUNORDPS

        # vmovups ymm2, [rip + vk_vec_zero]
        # vblendvps ymm3, ymm15, ymm0, ymm1
        vblendvps ymm3, ymm0, ymm15, ymm1


        vmovups [rdi + r14 + 0], ymm3



        add r13, 8
        cmp r12, r13
        ja .diffuse_loop_inner

    inc r10
    cmp r8, r10
    ja .diffuse_loop

    ret
