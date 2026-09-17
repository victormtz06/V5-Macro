package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.renderer.chunk.VisGraph;
import net.minecraft.core.BlockPos;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(VisGraph.class)
public class VisGraphMixin {
    @Inject(method = "setOpaque", at = @At("HEAD"), cancellable = true)
    private void v5$disableFreecamOcclusion(BlockPos pos, CallbackInfo ci) {
        if (Client.isFreecam()) ci.cancel();
    }
}
