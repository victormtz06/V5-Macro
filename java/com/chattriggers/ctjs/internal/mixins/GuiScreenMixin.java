package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.api.triggers.TriggerType;
import net.minecraft.client.gui.screens.Overlay;
import net.minecraft.client.gui.screens.Screen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

//? if <26.2 {
/*@Mixin(targets = "net.minecraft.client.Minecraft")
*///?} else {
@Mixin(targets = "net.minecraft.client.gui.Gui")
//?}
public class GuiScreenMixin {
    @Inject(method = "setScreen", at = @At("HEAD"))
    private void injectScreenOpened(Screen screen, CallbackInfo ci) {
        if (screen != null) {
            Client.automatedAttackHeld = false;
            TriggerType.GUI_OPENED.triggerAll(screen, ci);
        }
    }

    @Inject(method = "setOverlay", at = @At("HEAD"))
    private void injectOverlayOpened(Overlay overlay, CallbackInfo ci) {
        if (overlay != null) {
            Client.unpressKeys();
        }
    }
}
