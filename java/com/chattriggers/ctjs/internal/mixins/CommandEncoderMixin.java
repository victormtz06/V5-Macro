//? if >=26.2 {
package com.chattriggers.ctjs.internal.mixins;

import com.mojang.blaze3d.systems.CommandEncoder;
import com.mojang.blaze3d.systems.CommandEncoderBackend;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(CommandEncoder.class)
public interface CommandEncoderMixin {
    @Accessor("backend")
    CommandEncoderBackend ctjs$getBackend();
}
//?}
