package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.render.Render2D;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.client.gui.screens.Screen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Screen.class)
public class GuiRendererMixin {
    @Inject(method = "extractRenderStateWithTooltipAndSubtitles", at = @At("TAIL"))
    private void afterExtract(GuiGraphicsExtractor context, int mouseX, int mouseY, float deltaTicks, CallbackInfo ci) {
        Render2D.INSTANCE.runPreDrawables(context);
        Render2D.INSTANCE.runDrawables(context);
    }
}
