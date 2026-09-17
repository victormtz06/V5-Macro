package com.chattriggers.ctjs.internal.mixins;

import com.chattriggers.ctjs.internal.BoundKeyUpdater;
import net.minecraft.client.Options;
import net.minecraft.client.KeyMapping;
import com.mojang.blaze3d.platform.InputConstants;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(Options.class)
public class OptionsMixin implements BoundKeyUpdater {
    @Unique
    private Options.FieldAccess visitor;

    @Inject(method = "processOptions", at = @At("HEAD"))
    private void captureVisitor(Options.FieldAccess visitor, CallbackInfo ci) {
        this.visitor = visitor;
    }

    @Override
    public void ctjs_updateBoundKey(KeyMapping keyBinding) {
        String string = keyBinding.saveString();
        String string2 = visitor.process("key_" + keyBinding.saveString(), string);
        if (!string.equals(string2)) {
            keyBinding.setKey(InputConstants.getKey(string2));
        }
    }
}
