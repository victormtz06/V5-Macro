package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.Minecraft;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
import org.spongepowered.asm.mixin.gen.Invoker;

@Mixin(Minecraft.class)
public interface MinecraftAccessor {
    @Accessor
    void setMissTime(int missTime);

    @Invoker
    boolean invokeStartAttack();

    @Invoker
    void invokeStartUseItem();
}
