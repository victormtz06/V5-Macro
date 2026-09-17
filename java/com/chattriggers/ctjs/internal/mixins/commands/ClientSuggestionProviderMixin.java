package com.chattriggers.ctjs.internal.mixins.commands;

import com.chattriggers.ctjs.internal.CTClientCommandSource;
import net.minecraft.client.multiplayer.ClientSuggestionProvider;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Unique;

import java.util.HashMap;
import java.util.Map;

@SuppressWarnings("AddedMixinMembersNamePattern")
@Mixin(ClientSuggestionProvider.class)
public abstract class ClientSuggestionProviderMixin implements CTClientCommandSource {
    @Unique
    private final Map<String, Object> contextValues = new HashMap<>();

    @Override
    public Map<String, Object> getContextValues() {
        return contextValues;
    }
}
