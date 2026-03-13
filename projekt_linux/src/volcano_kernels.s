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
.global volcanoFlowFromDropAsm
volcanoFlowFromDropAsm:
    # TODO: scalar helper used by lava flow
    ret

# -----------------------------------------------------------------------------
# void volcanoParticleStepAsm(...)
# Pointer args arrive in:
# rdi=posX, rsi=posY, rdx=posZ, rcx=velX, r8=velY, r9=velZ
# Remaining scalar floats follow System V classification rules.
# -----------------------------------------------------------------------------
.global volcanoParticleStepAsm
volcanoParticleStepAsm:
    # TODO: scalar single-particle step for Linux ABI
    ret

# -----------------------------------------------------------------------------
# void volcanoParticleStepBatchAsm(...)
# -----------------------------------------------------------------------------
.global volcanoParticleStepBatchAsm
volcanoParticleStepBatchAsm:
    # TODO:
    # 1. split count into simdCount and scalar tail
    # 2. call volcanoParticleStepSIMDAsm for full 8-wide blocks
    # 3. finish remaining particles with scalar path
    ret

# -----------------------------------------------------------------------------
# void volcanoParticleStepSIMDAsm(...)
# -----------------------------------------------------------------------------
.global volcanoParticleStepSIMDAsm
volcanoParticleStepSIMDAsm:
    # TODO:
    # - load 8 particles (SoA)
    # - compute tempNorm, radius2, invR, ventPush
    # - update velX/velY/velZ
    # - apply damping
    # - update posX/posY/posZ
    # - update life and temp
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
