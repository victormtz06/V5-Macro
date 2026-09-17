package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.mojang.blaze3d.platform.InputConstants;
import net.minecraft.client.KeyboardHandler;
import net.minecraft.client.Minecraft;
import net.minecraft.client.Options;
import net.minecraft.client.input.KeyEvent;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.lwjgl.glfw.GLFW;

@Mixin(KeyboardHandler.class)
public class KeyboardHandlerMixin {
    @Inject(method = "keyPress", at = @At("HEAD"), cancellable = true)
    private void v5$cancelFreecamMovement(long window, int action, KeyEvent event, CallbackInfo ci) {
        if (action == GLFW.GLFW_RELEASE) {
            Client.releaseHeldKey(InputConstants.getKey(event));
        }

        Minecraft client = Minecraft.getInstance();
        if (!Client.isFreecam() || Client.getCurrentScreen() != null) {
            return;
        }

        Options options = client.options;
        if (options.keyUp.matches(event)
                || options.keyDown.matches(event)
                || options.keyLeft.matches(event)
                || options.keyRight.matches(event)
                || options.keyJump.matches(event)
                || options.keyShift.matches(event)
                || options.keySprint.matches(event)) {
            ci.cancel();
        }
    }
}
