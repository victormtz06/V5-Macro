package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.renderer.culling.Frustum;
import org.joml.FrustumIntersection;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(Frustum.class)
public class FrustumMixin {
    @Inject(method = "cubeInFrustum(DDDDDD)I", at = @At("HEAD"), cancellable = true)
    private void v5$showEverythingInFreecam(double minX, double minY, double minZ,
                                             double maxX, double maxY, double maxZ,
                                             CallbackInfoReturnable<Integer> cir) {
        if (Client.isFreecam()) cir.setReturnValue(FrustumIntersection.INSIDE);
    }
}
