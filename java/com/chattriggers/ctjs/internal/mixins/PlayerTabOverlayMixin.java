package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.api.triggers.TriggerType;
import com.chattriggers.ctjs.api.world.TabList;
import com.llamalad7.mixinextras.injector.ModifyReturnValue;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.components.PlayerTabOverlay;
import net.minecraft.world.scores.Scoreboard;
import net.minecraft.world.scores.Objective;
import net.minecraft.network.chat.Component;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(PlayerTabOverlay.class)
public class PlayerTabOverlayMixin {
    @ModifyReturnValue(method = "getNameForDisplay", at = @At("RETURN"))
    private Component v5$getPlayerName(Component original) {
        return Client.processName(original);
    }

    @Inject(method = "extractRenderState", at = @At("HEAD"), cancellable = true)
    private void injectRenderPlayerList(GuiGraphicsExtractor context, int scaledWindowWidth, Scoreboard scoreboard, Objective objective, CallbackInfo ci) {
        TriggerType.RENDER_PLAYER_LIST.triggerAll(ci);
    }

    @Inject(method = "setHeader", at = @At("HEAD"), cancellable = true)
    private void ctjs$keepCustomHeader(Component header, CallbackInfo ci) {
        if (TabList.INSTANCE.getCustomHeader$ctjs()) {
            ci.cancel();
        }
    }

    @Inject(method = "setFooter", at = @At("HEAD"), cancellable = true)
    private void ctjs$keepCustomFooter(Component footer, CallbackInfo ci) {
        if (TabList.INSTANCE.getCustomFooter$ctjs()) {
            ci.cancel();
        }
    }
}
