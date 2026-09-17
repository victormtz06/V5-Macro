//? if <26.2 {
/*package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.internal.listeners.WorldListener;
import com.llamalad7.mixinextras.sugar.Local;
import com.llamalad7.mixinextras.injector.ModifyExpressionValue;
import com.mojang.blaze3d.buffers.GpuBufferSlice;
import com.mojang.blaze3d.framegraph.FrameGraphBuilder;
import com.mojang.blaze3d.resource.GraphicsResourceAllocator;
import com.mojang.blaze3d.resource.ResourceHandle;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.renderer.LevelRenderer;
import net.minecraft.client.renderer.chunk.ChunkSectionsToRender;
import net.minecraft.client.renderer.culling.Frustum;
import net.minecraft.client.renderer.state.level.BlockOutlineRenderState;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import net.minecraft.client.renderer.state.level.LevelRenderState;
import net.minecraft.util.profiling.ProfilerFiller;
import org.joml.Matrix4fc;
import org.joml.Vector4f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.ModifyVariable;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(LevelRenderer.class)
public abstract class LevelRendererMixin {
    private float ctjs$tickDelta = 1.0F;

    @ModifyVariable(method = "cullTerrain", at = @At("HEAD"), argsOnly = true)
    private boolean v5$useSpectatorCullingInFreecam(boolean spectator) {
        return spectator || Client.isFreecam();
    }

    @Inject(method = "addMainPass", at = @At("HEAD"), cancellable = true)
    private void v5$renderMain(FrameGraphBuilder frameGraphBuilder, Frustum frustum, Matrix4fc matrix4f, GpuBufferSlice gpuBufferSlice, boolean renderBlockOutline, LevelRenderState worldRenderState, DeltaTracker deltaTracker, ProfilerFiller profiler, ChunkSectionsToRender chunkSectionsToRender, CallbackInfo ci) {
        if (Client.isMacroEnabled() && Client.getRenderLimiter() == Client.RenderLimiter.NO_RENDER) {
            ci.cancel();
        }
    }

    @Inject(
        method = "renderBlockOutline",
        at = @At(
            value = "FIELD",
            target = "Lnet/minecraft/client/renderer/state/level/CameraRenderState;pos:Lnet/minecraft/world/phys/Vec3;"
        ),
        cancellable = true
    )
    private void onDrawBlockOutline(CallbackInfo ci, @Local BlockOutlineRenderState outlineRenderState) {
        if (WorldListener.INSTANCE.triggerBlockOutline(outlineRenderState.pos()))
            ci.cancel();
    }

    @ModifyExpressionValue(
        method = "lambda$addMainPass$0",
        at = @At(value = "NEW", target = "()Lcom/mojang/blaze3d/vertex/PoseStack;")
    )
    private PoseStack onMatrixStack(PoseStack original) {
        WorldListener.INSTANCE.setMatrixStack(original);
        WorldListener.INSTANCE.triggerRenderStart(ctjs$tickDelta);
        return original;
    }

    @Inject(method = "renderLevel", at = @At("HEAD"))
    private void beforeRender(GraphicsResourceAllocator allocator, DeltaTracker tickCounter, boolean renderBlockOutline, CameraRenderState camera, Matrix4fc positionMatrix, GpuBufferSlice fogBuffer, Vector4f fogColor, boolean renderSky, ChunkSectionsToRender chunkSectionsToRender, CallbackInfo ci) {
        ctjs$tickDelta = tickCounter.getGameTimeDeltaTicks();
    }

    @Inject(
        method = "lambda$addMainPass$0",
        at = @At(value = "INVOKE", target = "Lnet/minecraft/client/renderer/LevelRenderer;finalizeGizmoCollection()V")
    )
    private void beforeGizmos(GpuBufferSlice gpuBufferSlice, LevelRenderState worldRenderState, ProfilerFiller profiler, ChunkSectionsToRender chunkSectionsToRender, ResourceHandle handle, ResourceHandle handle2, ResourceHandle handle3, ResourceHandle handle4, ResourceHandle handle5, boolean bl, Matrix4fc matrix4f, CallbackInfo ci) {
        WorldListener.INSTANCE.triggerRenderLast();
    }
}
*///?} else {
package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.internal.listeners.WorldListener;
import com.mojang.blaze3d.buffers.GpuBufferSlice;
import com.mojang.blaze3d.resource.GraphicsResourceAllocator;
import com.mojang.blaze3d.vertex.PoseStack;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.renderer.LevelRenderer;
import net.minecraft.client.renderer.SubmitNodeCollector;
import net.minecraft.client.renderer.rendertype.RenderType;
import net.minecraft.client.renderer.state.level.BlockOutlineRenderState;
import net.minecraft.client.renderer.state.level.CameraRenderState;
import org.joml.Matrix4fc;
import org.joml.Vector4f;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(LevelRenderer.class)
public abstract class LevelRendererMixin {
    private float ctjs$tickDelta = 1.0F;

    @Inject(method = "addMainPass", at = @At("HEAD"), cancellable = true)
    private void v5$renderMain(CallbackInfo ci) {
        if (Client.isMacroEnabled() && Client.getRenderLimiter() == Client.RenderLimiter.NO_RENDER) {
            ci.cancel();
        }
    }

    @Inject(method = "submitHitOutline", at = @At("HEAD"), cancellable = true)
    private void onDrawBlockOutline(
        PoseStack matrices,
        SubmitNodeCollector collector,
        RenderType renderType,
        BlockOutlineRenderState outline,
        int color,
        float lineWidth,
        boolean translucent,
        CallbackInfo ci
    ) {
        if (WorldListener.INSTANCE.triggerBlockOutline(outline.pos())) {
            ci.cancel();
        }
    }

    @Inject(method = "render", at = @At("HEAD"))
    private void beforeRender(
        GraphicsResourceAllocator allocator,
        DeltaTracker tickCounter,
        boolean renderBlockOutline,
        CameraRenderState camera,
        Matrix4fc positionMatrix,
        GpuBufferSlice fogBuffer,
        Vector4f fogColor,
        boolean renderSky,
        CallbackInfo ci
    ) {
        ctjs$tickDelta = tickCounter.getGameTimeDeltaTicks();
    }

    @Inject(method = "submitEntities", at = @At("HEAD"))
    private void beforeEntities(PoseStack matrices, net.minecraft.client.renderer.state.level.LevelRenderState state, SubmitNodeCollector collector, CallbackInfo ci) {
        WorldListener.INSTANCE.setMatrixStack(matrices);
        WorldListener.INSTANCE.triggerRenderStart(ctjs$tickDelta);
    }

    @Inject(method = "lambda$addMainPass$0", at = @At("RETURN"))
    private void afterMainPass(CallbackInfo ci) {
        WorldListener.INSTANCE.triggerRenderLast();
    }
}
//?}
