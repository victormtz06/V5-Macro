package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.api.client.Client;
import net.minecraft.client.gui.GuiGraphicsExtractor;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.MutableComponent;
import net.minecraft.network.chat.Style;
import net.minecraft.util.FormattedCharSequence;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyVariable;

@Mixin(GuiGraphicsExtractor.class)
public class GuiGraphicsExtractorMixin {
    @ModifyVariable(
            method = "text(Lnet/minecraft/client/gui/Font;Lnet/minecraft/util/FormattedCharSequence;IIIZ)V",
            at = @At("HEAD"),
            ordinal = 0,
            argsOnly = true)
    private FormattedCharSequence v5$processText(FormattedCharSequence original) {
        if (!Client.hasNameProcessor()) {
            return original;
        }

        MutableComponent component = Component.empty();
        StringBuilder text = new StringBuilder();
        Style[] currentStyle = {null};
        original.accept((index, style, codePoint) -> {
            if (currentStyle[0] != null && !currentStyle[0].equals(style)) {
                component.append(Component.literal(text.toString()).setStyle(currentStyle[0]));
                text.setLength(0);
            }
            currentStyle[0] = style;
            text.appendCodePoint(codePoint);
            return true;
        });
        if (currentStyle[0] != null) {
            component.append(Component.literal(text.toString()).setStyle(currentStyle[0]));
        }
        return Client.processName(component).getVisualOrderText();
    }
}
