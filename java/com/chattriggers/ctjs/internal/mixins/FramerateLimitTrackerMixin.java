package com.chattriggers.ctjs.internal.mixins;

import com.mojang.blaze3d.platform.FramerateLimitTracker;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(FramerateLimitTracker.class)
public class FramerateLimitTrackerMixin {
    @Shadow private int framerateLimit;

    @Inject(method = "getFramerateLimit", at = @At("HEAD"), cancellable = true)
    private void v5$removeTitleFramerateLimit(CallbackInfoReturnable<Integer> cir) {
        cir.setReturnValue(framerateLimit);
    }
}
