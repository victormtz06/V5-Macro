package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.api.render.Render2D;
import com.chattriggers.ctjs.internal.IRenderState;
import com.chattriggers.ctjs.internal.engine.CTEvents;
import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.client.renderer.entity.EntityRenderDispatcher;
import net.minecraft.client.renderer.entity.state.EntityRenderState;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.world.entity.Entity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(EntityRenderDispatcher.class)
public abstract class EntityRenderDispatcherMixin {

    @Inject(method = "onResourceManagerReload", at = @At("TAIL"))
    private void injectReload(net.minecraft.server.packs.resources.ResourceManager manager, CallbackInfo ci, @com.llamalad7.mixinextras.sugar.Local net.minecraft.client.renderer.entity.EntityRendererProvider.Context context) {
        Render2D.initializePlayerRenderers$ctjs(context);
    }

    @ModifyExpressionValue(
        method = "extractEntity",
        at = @At(value = "INVOKE", target = "Lnet/minecraft/client/renderer/entity/EntityRenderer;createRenderState(Lnet/minecraft/world/entity/Entity;F)Lnet/minecraft/client/renderer/entity/state/EntityRenderState;")
    )
    private EntityRenderState attachEntityToState(EntityRenderState state, Entity entity, float tickProgress) {
        ((IRenderState) state).ctjs$setEntity(entity);
        return state;
    }

    @Inject(method = "submit", at = @At("HEAD"), cancellable = true)
    private <S extends EntityRenderState> void injectRender(
            S renderState,
            CameraRenderState cameraRenderState,
            double d, double e, double f,
            PoseStack matrixStack,
            SubmitNodeCollector orderedRenderCommandQueue,
            CallbackInfo ci
    ) {
        Entity entity = ((IRenderState) renderState).ctjs$getEntity();

        if (entity != null) {
            float partialTicks = Client.getMinecraft().getDeltaTracker().getGameTimeDeltaTicks();
            CTEvents.RENDER_ENTITY.invoker().render(matrixStack, entity, partialTicks, ci);
        }
    }
}
