package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.CTJS;
import com.chattriggers.ctjs.internal.utils.NameReplacement;
import net.minecraft.client.gui.Font;
import net.minecraft.util.FormattedCharSequence;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.ModifyVariable;

@Mixin(Font.class)
public class FontMixin {
    @ModifyVariable(method = "prepareText(Lnet/minecraft/util/FormattedCharSequence;FFIZZI)Lnet/minecraft/client/gui/Font$PreparedText;",
            at = @At("HEAD"), argsOnly = true, ordinal = 0)
    private FormattedCharSequence v5$animateName(FormattedCharSequence original) {
        if (!CTJS.isLoaded()) return original;
        return NameReplacement.animate(NameReplacement.process(original));
    }
}
