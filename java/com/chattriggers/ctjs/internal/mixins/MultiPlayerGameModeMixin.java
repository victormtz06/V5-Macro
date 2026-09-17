package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.internal.engine.CTEvents;
import net.minecraft.client.multiplayer.MultiPlayerGameMode;
import net.minecraft.core.BlockPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(MultiPlayerGameMode.class)
public class MultiPlayerGameModeMixin {
    @Inject(
        method = "destroyBlock",
        at = @At(
            value = "INVOKE",
            target = "Lnet/minecraft/world/level/block/Block;destroy(Lnet/minecraft/world/level/LevelAccessor;Lnet/minecraft/core/BlockPos;Lnet/minecraft/world/level/block/state/BlockState;)V"
        )
    )
    private void injectBreakBlock(BlockPos pos, CallbackInfoReturnable<Boolean> cir) {
        CTEvents.BREAK_BLOCK.invoker().breakBlock(pos);
    }
}
