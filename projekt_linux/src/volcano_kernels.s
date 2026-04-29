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

.macro FLOW_FROM_DROP_AVX dst, drop, kval, tmp
    # TODO: implement vector version of flowFromDrop:
    # max(0, drop) -> shape -> result = pos * k * shape
    
    
    # dst = pos = max(0, drop)
    vmaxps \dst, \drop, YMMWORD PTR [rip + vk_vec_zero]

    # tmp = pos + eps
    vaddps \tmp, \dst, YMMWORD PTR [rip + vk_vec_drop_eps]

    # tmp = sqrt(tmp)
    vsqrtps \tmp, \tmp

    # tmp = tmp * scale
    vmulps \tmp, \tmp, YMMWORD PTR [rip + vk_vec_shape_scale]

    # tmp = tmp + bias
    vaddps \tmp, \tmp, YMMWORD PTR [rip + vk_vec_shape_bias]

    # dst = pos * k
    vmulps \dst, \dst, \kval

    # dst = dst * shape
    vmulps \dst, \dst, \tmp
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
    movss xmm13, xmm11
    mulss xmm11, xmm11 # x^2

    # mulss xmm12, xmm12 # y^2
    movss xmm13, [rdx] # z_i

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

    mulss xmm13, xmm10 # x_i * a_{vent} * invR
    addss xmm11, xmm13 # windX * (c_{w0} + c_{w1} * T_{norm}) + x_i * a_{vent} * invR =         a_x

    mulss xmm13, xmm10 # z_i * a_{vent} * invR
    mulss xmm12, xmm6 # windZ * (c_{w0} + c_{w1} * T_{norm})
    addss xmm12, xmm13 # windZ * (c_{w0} + c_{w1} * T_{norm}) + z_i * a_{vent} * invR =         a_z
    


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
#                        dt,
#                        gravity,
#                        plumeBuoyancy,
#                        ambientTemperature,
#                        damping,
#                        windX,
#                        windZ,
#                        particleCooling
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
    mov rdx, r13 # restore

    # r11 = loop limit
    # r12 = struct *
    mov r12, [rbp + 0x28]

.loop:
    # push args last to first
    push r12
    push qword ptr [rbp + 0x20]
    push qword ptr [rbp + 0x18]
    push qword ptr [rbp + 0x10]
    # call volcanoParticleStepSIMDAsm
    call volcanoParticleStepSIMDAsm
    # cleanup
    add rsp, 32



    add r10, 8
    # ab for unsigned
    cmp r11, r10
    ja .loop

    # process tail

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
    vmulps ymm0, ymm0, ymm0 # x^2
    vmulps ymm1, ymm1, ymm1 # z^2
    vaddps ymm0, ymm0, ymm1 # x^2 + z^2                                             
    vmovups ymm2, ymm0 #                                                            = r^2

    vmovups ymm1, [rip + vk_vec_eps]
    vminps ymm0, ymm1, ymm0 # max(r^2, \epsilon_r)

    vrsqrtps ymm0, ymm0 # 1/sqrt(max(r^2, \epslon_r))                               = invR

    mov r13, [rbp + 0x28] # struct dereference
    mov r14, [rbp + 0x18] # *temperature derefenrece

    # ambient temperature
    vbroadcastss ymm1, [r13 + 12] # ambient temperature
    vmovups ymm2, [r14] # temperatures
    vsubps ymm1, ymm2, ymm1 # T_i - T_{amb}
    vmovups ymm2, [rip + vk_vec_inv_temp]
    vdivps ymm1, ymm1, ymm2 # (T_i - T_{amb}) / T_{span}
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
    vsubps ymm5, ymm4, ymm2 # R^2 - r^2
    vmovups ymm4, [rip + vk_vec_zero]

    vmaxps ymm5, ymm5, ymm4 # max(0, R^2 - r^2)
    vmovups ymm4, [rip + vk_vec_vent_scale]
    vmulps ymm4, ymm4, ymm1 # K_{vent} * T_{norm}

    vmulps ymm4, ymm4, ymm5 # max(0, R^2 - r^2) * K_{vent} * T_{norm}               = a_{vent}



    vmovups ymm5, [rip + vk_vec_wind_scale]
    vmulps ymm5, ymm5, ymm1 # T_{norm} * c_{w1}
    vmovups ymm6, [rip + vk_vec_wind_bias]
    vaddps ymm5, ymm5, ymm6 # T_{norm} * c_{w1} + c_{w2}

    vbroadcastss ymm6, [r13 + 20] # windX
    vmulps ymm6, ymm5, ymm6 # windX * T_{norm} * c_{w1} + c_{w2}                    = a_x

    vbroadcastss ymm7, [r13 + 24] # windZ
    vmulps ymm5, ymm5, ymm7 # windZ * T_{norm} * c_{w1} + c_{w2}                    = a_z

    
    vbroadcastss ymm7, [r13 + 0] # dt
    vmovups ymm8, ymm7
    vmulps ymm3, ymm3, ymm7 # a_y * dt
    vmulps ymm6, ymm6, ymm7 # a_x * dt
    vmulps ymm5, ymm5, ymm7 # a_z * dt

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

    vmulps ymm3, ymm3, ymm8 # v_y * dt
    vmulps ymm6, ymm6, ymm8 # v_x * dt
    vmulps ymm5, ymm5, ymm8 # v_z * dt


    vmovups ymm7, [rsi]
    vaddps ymm7, ymm7, ymm3
    vmovups [rsi], ymm7

    vmovups ymm7, [rdi]
    vaddps ymm7, ymm7, ymm6
    vmovups [rdi], ymm7

    vmovups ymm7, [rdx]
    vaddps ymm7, ymm7, ymm5
    vmovups [rdx], ymm7

    




    # vmulps ymm0, ymm0, 



    
    # vbroadcastps ymm, [rdi] # x_i



    pop rbp
    ret

# -----------------------------------------------------------------------------
# void volcanoUpdateLavaFluxSIMDAsm(...)
# -----------------------------------------------------------------------------
.global volcanoUpdateLavaFluxSIMDAsm
volcanoUpdateLavaFluxSIMDAsm:
    # TODO:
    # - iterate interior cells by rows
    # - process 8 cells at once
    # - compute fluidity, k coefficients, in/out flow
    # - clamp output and write lavaHeightNext
    ret

# -----------------------------------------------------------------------------
# void volcanoDiffuseHeatSIMDAsm(...)
# -----------------------------------------------------------------------------
.global volcanoDiffuseHeatSIMDAsm
volcanoDiffuseHeatSIMDAsm:
    # TODO:
    # - iterate interior cells by rows
    # - compute laplacian
    # - add lava heating, subtract cooling
    # - clamp to [ambientTemperature, 1520]
    ret
