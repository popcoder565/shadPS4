// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static void LowerCmpClass(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::F32 value = IR::F32{inst.Arg(0)};
    if (inst.Arg(1).IsImmediate()) {
        const auto class_mask = static_cast<IR::FloatClassFunc>(inst.Arg(1).U32());
        if (class_mask == IR::FloatClassFunc::NaN) {
            inst.ReplaceOpcode(IR::Opcode::FPIsNan32);
            return;
        }
        if (class_mask == IR::FloatClassFunc::Infinity) {
            inst.ReplaceOpcode(IR::Opcode::FPIsInf32);
            return;
        }
        if (class_mask == IR::FloatClassFunc::Finite) {
            inst.ReplaceUsesWithAndRemove(
                ir.LogicalNot(ir.LogicalOr(ir.FPIsNan(value), ir.FPIsInf(value))));
            return;
        }
    }

    // Negative zero, denormal, normal and infinity are mask bits 5 to 2, mirroring the positive
    // classes in bits 6 to 9. NaN classes don't depend on the sign.
    const IR::U32 bits = ir.BitCast<IR::U32>(value);
    const IR::U32 magnitude = ir.BitwiseAnd(bits, ir.Imm32(0x7fffffffu));
    const auto magnitude_below = [&](u32 limit) {
        return ir.ILessThan(magnitude, ir.Imm32(limit), false);
    };
    const IR::U32 nan_class =
        IR::U32{ir.Select(magnitude_below(0x7fc00000u), ir.Imm32(0u), ir.Imm32(1u))};
    const IR::U32 inf_class =
        IR::U32{ir.Select(ir.IEqual(magnitude, ir.Imm32(0x7f800000u)), ir.Imm32(9u), nan_class)};
    const IR::U32 normal_class =
        IR::U32{ir.Select(magnitude_below(0x7f800000u), ir.Imm32(8u), inf_class)};
    const IR::U32 denorm_class =
        IR::U32{ir.Select(ir.IEqual(magnitude, ir.Imm32(0u)), ir.Imm32(6u), ir.Imm32(7u))};
    const IR::U32 positive_class =
        IR::U32{ir.Select(magnitude_below(0x00800000u), denorm_class, normal_class)};
    const IR::U1 negative =
        ir.LogicalAnd(ir.ILessThan(bits, ir.Imm32(0u), true),
                      ir.ILessThanEqual(magnitude, ir.Imm32(0x7f800000u), false));
    const IR::U32 class_index =
        IR::U32{ir.Select(negative, ir.ISub(ir.Imm32(11u), positive_class), positive_class)};
    const IR::U32 class_bit = IR::U32{ir.ShiftLeftLogical(ir.Imm32(1u), class_index)};
    inst.ReplaceUsesWithAndRemove(
        ir.INotEqual(ir.BitwiseAnd(IR::U32{inst.Arg(1)}, class_bit), ir.Imm32(0u)));
}

static void LowerPackedAncillary(IR::Inst& value) {
    boost::container::small_vector<IR::Inst*, 4> extracts;
    for (const auto& [user, operand] : value.Uses()) {
        if (user->GetOpcode() == IR::Opcode::BitFieldUExtract && operand == 0 &&
            user->Arg(1).IsImmediate() && user->Arg(2).IsImmediate()) {
            extracts.push_back(user);
        }
    }
    for (IR::Inst* const inst : extracts) {
        const u32 offset = inst->Arg(1).U32();
        const u32 bits = inst->Arg(2).U32();
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        if (offset >= 8 && offset + bits <= 12) {
            const auto sample_index = ir.GetAttributeU32(IR::Attribute::SampleIndex);
            if (offset == 8 && bits == 4) {
                inst->ReplaceUsesWithAndRemove(sample_index);
            } else {
                inst->ReplaceUsesWithAndRemove(
                    ir.BitFieldExtract(sample_index, ir.Imm32(offset - 8), ir.Imm32(bits)));
            }
        } else if (offset >= 16 && offset + bits <= 27) {
            const auto mrt_index = ir.GetAttributeU32(IR::Attribute::RenderTargetIndex);
            if (offset == 16 && bits == 11) {
                inst->ReplaceUsesWithAndRemove(mrt_index);
            } else {
                inst->ReplaceUsesWithAndRemove(
                    ir.BitFieldExtract(mrt_index, ir.Imm32(offset - 16), ir.Imm32(bits)));
            }
        } else {
            UNREACHABLE_MSG("Unhandled bitfield extract from ancillary VGPR offset={}, bits={}",
                            offset, bits);
        }
    }
    value.ReplaceUsesWithAndRemove(IR::Value{0U});
}

static void LowerMaskedBitCount(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::U32 thread_mask{
        ir.GetAttributeU32(IR::Attribute::SubgroupLtMask, inst.Arg(2).U1() ? 1 : 0)};
    const IR::U32 masked_value{ir.BitCount(ir.BitwiseAnd(IR::U32{inst.Arg(0)}, thread_mask))};
    inst.ReplaceUsesWithAndRemove(ir.IAdd(masked_value, IR::U32{inst.Arg(1)}));
}

static void Lower(IR::Block& block, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::FPCmpClass32:
        return LowerCmpClass(block, inst);
    case IR::Opcode::GetAttributeU32:
        if (inst.Arg(0).Attribute() == IR::Attribute::PackedAncillary) {
            LowerPackedAncillary(inst);
        }
        break;
    case IR::Opcode::MaskedBitCount32:
        return LowerMaskedBitCount(block, inst);
    default:
        break;
    }
}

void LowerHardwareIntrinsics(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Lower(*block, inst);
        }
    }
}

} // namespace Shader::Optimization
