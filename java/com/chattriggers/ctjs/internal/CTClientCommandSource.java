package com.chattriggers.ctjs.internal;

import net.minecraft.commands.SharedSuggestionProvider;

import java.util.Map;

public interface CTClientCommandSource extends SharedSuggestionProvider {
    Map<String, Object> getContextValues();
}
