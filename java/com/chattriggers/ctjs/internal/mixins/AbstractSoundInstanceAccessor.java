package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.resources.sounds.AbstractSoundInstance;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(AbstractSoundInstance.class)
public interface AbstractSoundInstanceAccessor {
    @Accessor
    void setLooping(boolean repeat);

    @Accessor
    void setDelay(int delay);
}
