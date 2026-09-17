package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.ProxyManagerScreen;
import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.client.gui.screens.multiplayer.JoinMultiplayerScreen;
import net.minecraft.client.gui.components.Button;
import net.minecraft.network.chat.Component;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(JoinMultiplayerScreen.class)
public abstract class JoinMultiplayerScreenMixin extends Screen {

    @Unique
    private Button v5_proxyButton;

    protected JoinMultiplayerScreenMixin(Component text) {
        super(text);
    }

    @Inject(method = "init", at = @At("TAIL"))
    private void init(CallbackInfo ci) {
        this.v5_proxyButton = Button.builder(Component.literal("V5 Proxies"), b -> {
                    if (this.minecraft != null) {
                        Client.setCurrentScreen(new ProxyManagerScreen(this));
                    }
                })
                .bounds(0, 5, 80, 20)
                .build();

        this.addRenderableWidget(this.v5_proxyButton);
    }

    @Override
    public void extractRenderState(GuiGraphicsExtractor context, int mouseX, int mouseY, float delta) {
        if (this.v5_proxyButton != null) {
            this.v5_proxyButton.setX(this.width - 80 - 5);
        }

        super.extractRenderState(context, mouseX, mouseY, delta);
    }
}
