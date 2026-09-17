package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.internal.engine.CTEvents;
import net.minecraft.world.level.block.entity.BlockEntity;
import net.minecraft.client.renderer.blockentity.state.BlockEntityRenderState;
import net.minecraft.client.renderer.feature.ModelFeatureRenderer;
import com.mojang.blaze3d.vertex.PoseStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(BlockEntityRenderState.class)
public class BlockEntityRenderStateMixin {

    @Unique
    private static final PoseStack stack = new PoseStack();

    @Inject(
        method = "extractBase(Lnet/minecraft/world/level/block/entity/BlockEntity;Lnet/minecraft/client/renderer/blockentity/state/BlockEntityRenderState;Lnet/minecraft/client/renderer/feature/ModelFeatureRenderer$CrumblingOverlay;)V",
        at = @At("HEAD")
    )
    private static void onUpdate(BlockEntity blockEntity, BlockEntityRenderState state, ModelFeatureRenderer.CrumblingOverlay crumblingOverlay, CallbackInfo ci) {
        if (blockEntity.getLevel() != null && blockEntity.getLevel().isClientSide()) {

            stack.pushPose();

            CTEvents.RENDER_BLOCK_ENTITY.invoker().render(
                stack,
                blockEntity,
                Client.getMinecraft().getDeltaTracker().getGameTimeDeltaTicks(),
                ci
            );

            stack.popPose();
        }
    }
}
