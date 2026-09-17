package com.chattriggers.ctjs.internal.mixins;

import net.minecraft.client.KeyMapping;
import com.mojang.blaze3d.platform.InputConstants;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

import java.util.List;

@Mixin(KeyMapping.class)
public interface KeyMappingAccessor {
    @Accessor
    InputConstants.Key getKey();

    @Accessor
    int getClickCount();

    @Mixin(net.minecraft.client.KeyMapping.Category.class)
    interface Category {
        @Accessor("SORT_ORDER")
        static List<net.minecraft.client.KeyMapping.Category> getCategoryList() { throw new IllegalStateException(); }
    }
}
