//? if >=26.2 {
package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.renderer.GameRenderer;
import net.minecraft.client.renderer.SubmitNodeStorage;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(GameRenderer.class)
public interface GameRendererAccessor {
    @Accessor
    SubmitNodeStorage getHandAndScreenSubmitNodeStorage();
}
//?}
