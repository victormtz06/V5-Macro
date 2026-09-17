package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.internal.engine.CTEvents;
import com.chattriggers.ctjs.internal.listeners.MouseListener;
import com.llamalad7.mixinextras.sugar.Local;
import com.mojang.blaze3d.platform.InputConstants;
import net.minecraft.client.Minecraft;
import net.minecraft.client.MouseHandler;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.input.MouseButtonInfo;
import net.minecraft.client.player.LocalPlayer;
import net.minecraft.util.Mth;
import org.objectweb.asm.Opcodes;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.Redirect;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.lwjgl.glfw.GLFW;

@Mixin(MouseHandler.class)
public class MouseHandlerMixin {
    @Shadow
    private MouseButtonInfo activeButton;

    private int v5$guiMouseButton = -1;

    private boolean v5$cameraLookEnabled() {
        return Client.isFreecam() || Client.isFreelook();
    }

    @Inject(method = "onButton", at = @At("HEAD"), cancellable = true)
    private void v5$cancelFreecamClick(long window, MouseButtonInfo button, int action, CallbackInfo ci) {
        Minecraft client = Minecraft.getInstance();
        MouseListener.onRawMouseInput(button.button(), action);
        if (action == GLFW.GLFW_PRESS && Client.getCurrentScreen() != null) {
            v5$guiMouseButton = button.button();
        } else if (action == GLFW.GLFW_RELEASE) {
            if (button.button() == v5$guiMouseButton) {
                v5$guiMouseButton = -1;
            } else if (!v5$cameraLookEnabled() || (button.button() != 0 && button.button() != 1)) {
                Client.releaseHeldKey(InputConstants.Type.MOUSE.getOrCreate(button.button()));
            }
        }

        if (v5$cameraLookEnabled()
            && Client.getCurrentScreen() == null
            && (button.button() == 0 || button.button() == 1)) {
            ci.cancel();
        }
    }

    @Redirect(
        method = "turnPlayer(D)V",
        at = @At(
            value = "INVOKE",
            target = "Lnet/minecraft/client/player/LocalPlayer;turn(DD)V"
        )
    )
    private void v5$turnFreecam(LocalPlayer player, double yawDelta, double pitchDelta) {
        if (!v5$cameraLookEnabled()) {
            player.turn(yawDelta, pitchDelta);
            return;
        }

        Float yaw = Client.getCameraYaw();
        Float pitch = Client.getCameraPitch();
        Client.setCameraRotation(
            (yaw != null ? yaw : player.getYRot()) + (float) yawDelta * 0.15F,
            Mth.clamp((pitch != null ? pitch : player.getXRot()) + (float) pitchDelta * 0.15F, -90.0F, 90.0F)
        );
    }

    @Inject(method = "grabMouse()V", at = @At("HEAD"), cancellable = true)
    private void v5$lockCursor(CallbackInfo ci) {
        if (Client.isUngrabbed()) {
            Client.resumeHeldKeys();
            ci.cancel();
        }
    }

    @Inject(method = "grabMouse()V", at = @At("TAIL"))
    private void v5$restoreHeldKeys(CallbackInfo ci) {
        Client.resumeHeldKeys();
    }

    @Inject(method = "turnPlayer(D)V", at = @At("HEAD"), cancellable = true)
    private void v5$updateMouse(CallbackInfo ci) {
        if (Client.isUngrabbed()) {
            ci.cancel();
        }
    }

    @Inject(method = "onScroll(JDD)V", at = @At("HEAD"), cancellable = true)
    private void v5$onMouseScroll(long window, double horizontalScroll, double verticalScroll, CallbackInfo ci) {
        Minecraft client = Minecraft.getInstance();
        if (Client.isFreelook() && Client.getCurrentScreen() == null) {
            Client.setFreelookDistance(Client.getFreelookDistance() - verticalScroll);
            ci.cancel();
            return;
        }

        if (!Client.isInputLocked()) {
            return;
        }

        if (Client.getCurrentScreen() == null) {
            ci.cancel();
        }
    }


    @Inject(
        method = "onScroll",
        at = @At(
            value = "FIELD",
            target = "Lnet/minecraft/client/Minecraft;options:Lnet/minecraft/client/Options;",
            opcode = Opcodes.GETFIELD
        )
    )
    private void injectOnMouseScroll(long window, double horizontal, double vertical, CallbackInfo ci) {
        MouseListener.onRawMouseScroll(vertical);
    }

    @Inject(
        method = "handleAccumulatedMovement",
        at = @At(
            value = "INVOKE",
            target = "Lnet/minecraft/client/gui/screens/Screen;mouseDragged(Lnet/minecraft/client/input/MouseButtonEvent;DD)Z"
        ),
        cancellable = true
    )
    private void injectOnGuiMouseDrag(
        CallbackInfo ci,
        @Local(ordinal = 0) double d,
        @Local(ordinal = 1) double e,
        @Local Screen screen,
        @Local(ordinal = 2) double f,
        @Local(ordinal = 3) double g)
    {
        if (screen != null) {
            CTEvents.GUI_MOUSE_DRAG.invoker().process(f, g, d, e, activeButton.button(), screen, ci);
        }
    }
}
