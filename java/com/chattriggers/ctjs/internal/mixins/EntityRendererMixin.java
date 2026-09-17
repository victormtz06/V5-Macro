package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.renderer.entity.EntityRenderer;
import net.minecraft.network.chat.Component;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

@Mixin(EntityRenderer.class)
public class EntityRendererMixin {
    @Inject(method = "getNameTag", at = @At("RETURN"), cancellable = true)
    private void v5$getDisplayName(CallbackInfoReturnable<Component> cir) {
        Component original = cir.getReturnValue();
        if (original == null) {
            return;
        }

        Component updated = Client.processName(original);
        if (updated != original) {
            cir.setReturnValue(updated);
        }
    }
}
