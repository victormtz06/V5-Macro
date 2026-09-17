package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.gui.screens.inventory.BookViewScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(BookViewScreen.class)
public interface BookViewScreenAccessor {
    @Accessor
    int getCurrentPage();

    @Invoker
    void invokeUpdateButtonVisibility();
}
