package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import com.chattriggers.ctjs.api.render.DrawContextHolder;
import com.chattriggers.ctjs.api.render.Render2D;
import com.chattriggers.ctjs.api.world.Scoreboard;
import com.chattriggers.ctjs.internal.engine.CTEvents;
import gg.essential.universal.UMatrixStack;
import net.minecraft.client.DeltaTracker;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

//? if <26.2 {
/*@Mixin(targets = "net.minecraft.client.gui.Gui")
*///?} else {
@Mixin(targets = "net.minecraft.client.gui.Hud")
//?}
public class GuiHudMixin {
    @Inject(method = "extractRenderState", at = @At("HEAD"))
    private void captureContext(GuiGraphicsExtractor context, DeltaTracker tickCounter, CallbackInfo ci) {
        DrawContextHolder.currentContext = context;
        if (Client.getCurrentScreen() == null) {
            Render2D.INSTANCE.runPreDrawables(context);
        }
    }

    @Inject(method = "extractScoreboardSidebar", at = @At("HEAD"), cancellable = true)
    private void injectRenderScoreboard(GuiGraphicsExtractor context, DeltaTracker tickCounter, CallbackInfo ci) {
        if (!Scoreboard.getShouldRender()) {
            ci.cancel();
        }
    }

    @Inject(method = "extractRenderState", at = @At("TAIL"))
    private void injectRenderOverlay(GuiGraphicsExtractor context, DeltaTracker tickCounter, CallbackInfo ci) {
        CTEvents.RENDER_OVERLAY.invoker().render(
            context,
            new UMatrixStack(context.pose()).toMC(),
            tickCounter.getGameTimeDeltaTicks()
        );
        if (Client.getCurrentScreen() == null) {
            Render2D.INSTANCE.runDrawables(context);
        }
    }
}
