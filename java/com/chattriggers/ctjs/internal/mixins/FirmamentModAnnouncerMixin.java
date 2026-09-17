package com.chattriggers.ctjs.internal.mixins;

import com.llamalad7.mixinextras.injector.wrapmethod.WrapMethod;
import com.llamalad7.mixinextras.injector.wrapoperation.Operation;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Pseudo;
import org.spongepowered.asm.mixin.injection.Coerce;

@Pseudo
@Mixin(targets = "moe.nea.firmament.features.misc.ModAnnouncer")
public class FirmamentModAnnouncerMixin {
    @WrapMethod(method = "onServerJoin", require = 0)
    private void onServerJoin(@Coerce Object event, Operation<Void> original) {
        // Suppress Firmament's mod announcer.
    }
}
