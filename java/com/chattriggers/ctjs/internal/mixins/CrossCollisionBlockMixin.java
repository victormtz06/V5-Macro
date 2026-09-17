package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.world.level.block.CrossCollisionBlock;
import net.minecraft.world.level.block.StainedGlassPaneBlock;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(CrossCollisionBlock.class)
public class CrossCollisionBlockMixin {

    @Inject(method = "getShape", at = @At("HEAD"), cancellable = true)
    private void v5$getShape(CallbackInfoReturnable<VoxelShape> cir) {
        if ((Object) this instanceof StainedGlassPaneBlock) {
            cir.setReturnValue(Shapes.block());
        }
    }
}
