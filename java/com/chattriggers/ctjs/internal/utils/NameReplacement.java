package com.chattriggers.ctjs.internal.utils;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import net.minecraft.ChatFormatting;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.MutableComponent;
import net.minecraft.network.chat.Style;
import net.minecraft.network.chat.TextColor;
import net.minecraft.util.FormattedCharSequence;

import java.awt.Color;
import java.util.IdentityHashMap;
import java.util.Objects;
import java.util.Optional;

public final class NameReplacement {
    private static String username;
    private static String input;
    private static Component replacement;
    private static final Cache<FormattedCharSequence, FormattedCharSequence> sequences =
            CacheBuilder.newBuilder().weakKeys().maximumSize(2048).build();
    private static final TextColor[] chroma = new TextColor[40];
    private static final IdentityHashMap<TextColor, Integer> phases = new IdentityHashMap<>();
    private static final Cache<TextColor, Boolean> replacementColors = CacheBuilder.newBuilder().weakKeys().build();

    static {
        for (int i = 0; i < chroma.length; i++) {
            chroma[i] = TextColor.fromRgb(Color.HSBtoRGB(i / 40f, 0.8f, 1f) & 0xffffff);
            phases.put(chroma[i], i);
            replacementColors.put(chroma[i], true);
        }
    }

    public static void configure(String name, String customName) {
        if (Objects.equals(username, name) && Objects.equals(input, customName)) return;
        username = name;
        input = customName;
        replacement = customName == null ? Component.empty() : createReplacement(customName);
        sequences.invalidateAll();
    }

    public static boolean isActive() {
        return username != null && !username.isEmpty();
    }

    private static Component createReplacement(String text) {
        if (text.matches("#[0-9a-fA-F]{6}.+")) {
            return Component.literal(text.substring(7)).withStyle(
                    Style.EMPTY.withColor(markColor(Integer.parseInt(text.substring(1, 7), 16))));
        }
        MutableComponent result = Component.empty();
        boolean rainbow = !text.contains("&") && !text.contains("§");
        Style style = Style.EMPTY;
        int character = 0;
        for (int i = 0; i < text.length();) {
            int codePoint = text.codePointAt(i);
            i += Character.charCount(codePoint);
            if (!rainbow && (codePoint == '&' || codePoint == '§') && i < text.length()) {
                ChatFormatting format = ChatFormatting.getByCode(text.charAt(i));
                if (format != null) {
                    style = style.applyLegacyFormat(format);
                    i++;
                    continue;
                }
            }
            Style glyphStyle = rainbow ? style.withBold(true).withColor(chroma[character++ % 40])
                    : style.withColor((TextColor) null).withColor(
                            markColor(style.getColor() == null ? 0xffffff : style.getColor().getValue()));
            result.append(Component.literal(new String(Character.toChars(codePoint))).setStyle(glyphStyle));
        }
        return result;
    }

    private static TextColor markColor(int rgb) {
        TextColor color = TextColor.fromRgb(rgb);
        replacementColors.put(color, true);
        return color;
    }

    private static boolean isReplacement(Style style) {
        return style.getColor() != null && replacementColors.getIfPresent(style.getColor()) != null;
    }

    public static Component process(Component original) {
        if (!isActive()) return original;
        String plain = original.getString();
        if (!plain.contains(username)) return original;
        StringBuilder searchable = new StringBuilder();
        original.visit((style, text) -> {
            searchable.append(isReplacement(style) ? "\0".repeat(text.length()) : text);
            return Optional.empty();
        }, Style.EMPTY);
        String source = searchable.toString();
        if (!source.contains(username)) return original;
        MutableComponent result = Component.empty();
        int[] position = {0};
        int[] next = {source.indexOf(username)};
        int[] skipUntil = {0};
        original.visit((style, text) -> {
            int start = position[0];
            int end = start + text.length();
            int cursor = Math.max(start, skipUntil[0]);
            while (next[0] >= 0 && next[0] < end) {
                if (cursor < next[0]) {
                    result.append(Component.literal(text.substring(cursor - start, next[0] - start)).setStyle(style));
                }
                result.append(replacement.copy().withStyle(style));
                skipUntil[0] = next[0] + username.length();
                cursor = skipUntil[0];
                next[0] = source.indexOf(username, cursor);
            }
            if (cursor < end) result.append(Component.literal(text.substring(cursor - start)).setStyle(style));
            position[0] = end;
            return Optional.empty();
        }, Style.EMPTY);
        return result;
    }

    public static FormattedCharSequence process(FormattedCharSequence original) {
        if (!isActive()) return original;
        FormattedCharSequence cached = sequences.getIfPresent(original);
        if (cached != null) return cached;
        StringBuilder plain = new StringBuilder();
        original.accept((index, style, codePoint) -> {
            plain.appendCodePoint(isReplacement(style) ? 0 : codePoint);
            return true;
        });
        FormattedCharSequence result = original;
        if (plain.indexOf(username) >= 0) {
            MutableComponent component = Component.empty();
            original.accept((index, style, codePoint) -> {
                component.append(Component.literal(new String(Character.toChars(codePoint))).setStyle(style));
                return true;
            });
            result = process(component).getVisualOrderText();
        }
        sequences.put(original, result);
        return result;
    }

    public static FormattedCharSequence animate(FormattedCharSequence original) {
        if (!isActive()) return original;
        float hue = (System.currentTimeMillis() % 2000) / 2000f;
        return sink -> original.accept((index, style, codePoint) -> {
            Integer phase = phases.get(style.getColor());
            return sink.accept(index, phase == null ? style : style.withColor(
                    Color.HSBtoRGB(hue + phase / 40f, 0.8f, 1f) & 0xffffff), codePoint);
        });
    }
}
