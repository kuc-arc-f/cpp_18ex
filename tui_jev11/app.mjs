#!/usr/bin/env node

// app.jsx
import React, { useState, useEffect, useRef, useCallback } from "react";
import { render, Box, Text, useInput, useApp, useStdout, useStdin } from "ink";
import TextInput from "ink-text-input";
import { jsx, jsxs } from "react/jsx-runtime";
import koffi from "koffi"
import 'dotenv/config'

var COLORS = {
  primary: "#7C3AED",
  // violet
  accent: "#06B6D4",
  // cyan
  success: "#10B981",
  // emerald
  warning: "#F59E0B",
  // amber
  error: "#EF4444",
  // red
  muted: "#6B7280",
  // gray
  userBg: "#1E1B4B",
  // deep indigo
  aiBg: "#064E3B",
  // deep green
  border: "#374151",
  highlight: "#FBBF24"
};
var SPINNER_FRAMES = ["\u280B", "\u2819", "\u2839", "\u2838", "\u283C", "\u2834", "\u2826", "\u2827", "\u2807", "\u280F"];
var AI_NAME = "RAG-App";
var USER_NAME = "You";
var AI_RESPONSES = [
  (q) => `Sure! Here's what I think about "${q}":

\`\`\`javascript
// Example code
const answer = await solve("${q.slice(0, 20)}...");
console.log(answer);
\`\`\`

Would you like me to explain further?`,
  (q) => `Great question! Let me break down **"${q}"** step by step:

1. First, analyze the problem
2. Design a solution
3. Implement & test

I'm ready to dive deeper if needed.`,
  (q) => `I understand you're asking about: *${q}*

Here's a concise answer:
\u2192 The key insight is to think recursively.
\u2192 Time complexity: O(n log n)
\u2192 Space complexity: O(n)`,
  () => `That's an interesting challenge! Here's my approach:

\`\`\`python
def solution(input):
    # Process input
    result = transform(input)
    return result
\`\`\`

Let me know if you want a different language.`,
  (q) => `Processing "${q}"...

\u2713 Syntax checked
\u2713 Dependencies resolved
\u2713 Output generated

Done! The operation completed successfully.`
];
function getAIResponse(question) {
  const idx = Math.floor(Math.random() * AI_RESPONSES.length);
  return AI_RESPONSES[idx](question);
}
function Spinner({ label = "Thinking" }) {
  const [frame, setFrame] = useState(0);
  useEffect(() => {
    const id = setInterval(() => setFrame((f) => (f + 1) % SPINNER_FRAMES.length), 80);
    return () => clearInterval(id);
  }, []);
  return /* @__PURE__ */ jsxs(Box, { children: [
    /* @__PURE__ */ jsxs(Text, { color: COLORS.accent, children: [
      SPINNER_FRAMES[frame],
      " "
    ] }),
    /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: label }),
    /* @__PURE__ */ jsx(Text, { color: COLORS.accent, children: "..." })
  ] });
}
function MessageBubble({ message, terminalWidth }) {
  const isUser = message.role === "user";
  const maxWidth = Math.min(terminalWidth - 6, 80);
  const lines = message.content.split("\n");
  return /* @__PURE__ */ jsxs(
    Box,
    {
      flexDirection: "column",
      marginY: 0,
      paddingX: 1,
      paddingY: 0,
      children: [
        /* @__PURE__ */ jsxs(Box, { flexDirection: "row", gap: 1, marginBottom: 0, children: [
          /* @__PURE__ */ jsx(Text, { color: isUser ? COLORS.accent : COLORS.success, bold: true, children: isUser ? `\u25C8 ${USER_NAME}` : `\u2726 ${AI_NAME}` }),
          /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: message.timestamp })
        ] }),
        /* @__PURE__ */ jsx(
          Box,
          {
            flexDirection: "column",
            marginLeft: 2,
            borderStyle: "round",
            borderColor: isUser ? COLORS.accent : COLORS.success,
            paddingX: 1,
            paddingY: 0,
            //width: maxWidth,
            children: lines.map((line, i) => {
              if (line.startsWith("```")) {
                return /* @__PURE__ */ jsx(Text, { color: COLORS.warning, bold: true, children: line }, i);
              }
              if (line.includes("`") && !line.startsWith("```")) {
                return /* @__PURE__ */ jsx(Text, { color: isUser ? "#E0E7FF" : "#D1FAE5", children: line }, i);
              }
              if (/^\d+\./.test(line) || line.startsWith("\u2192") || line.startsWith("\u2713") || line.startsWith("\u2717")) {
                return /* @__PURE__ */ jsx(Text, { color: COLORS.highlight, children: line }, i);
              }
              if (line.startsWith("**") || line.startsWith("*")) {
                return /* @__PURE__ */ jsx(Text, { color: isUser ? "#C7D2FE" : "#A7F3D0", bold: true, children: line.replace(/\*\*/g, "").replace(/\*/g, "") }, i);
              }
              return /* @__PURE__ */ jsx(Text, { color: isUser ? "#E0E7FF" : "#D1FAE5", children: line || " " }, i);
            })
          }
        )
      ]
    }
  );
}
function Header({ terminalWidth }) {
  const title = "  \u25C6 RAG-App  ";
  const subtitle = " AI Terminal Chat ";
  const sep = "\u2500".repeat(Math.max(0, terminalWidth - 4));
  return /* @__PURE__ */ jsxs(Box, { flexDirection: "column", children: [
    /* @__PURE__ */ jsxs(
      Box,
      {
        borderStyle: "double",
        borderColor: COLORS.primary,
        paddingX: 2,
        paddingY: 0,
        justifyContent: "center",
        children: [
          /* @__PURE__ */ jsx(Text, { color: COLORS.primary, bold: true, children: title }),
          /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: subtitle }),
          /* @__PURE__ */ jsx(Text, { color: COLORS.accent, bold: true, children: "v1.0" })
        ]
      }
    ),
    /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: sep })
  ] });
}
function StatusBar({ messageCount, isLoading, terminalWidth }) {
  const sep = "\u2500".repeat(Math.max(0, terminalWidth - 4));
  return /* @__PURE__ */ jsxs(Box, { flexDirection: "column", children: [
    /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: sep }),
    /* @__PURE__ */ jsxs(Box, { flexDirection: "row", justifyContent: "space-between", paddingX: 1, children: [
      /* @__PURE__ */ jsxs(Box, { flexDirection: "row", gap: 2, children: [
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: "Messages: " }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.accent, children: messageCount })
      ] }),
      /* @__PURE__ */ jsx(Box, { flexDirection: "row", gap: 1, children: /* @__PURE__ */ jsx(Text, { color: isLoading ? COLORS.warning : COLORS.success, children: isLoading ? "\u25CF Generating" : "\u25CF Ready" }) }),
      /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: "[Ctrl+C] Quit  [Enter] Send" })
    ] })
  ] });
}
function InputArea({ value, onChange, onSubmit, isLoading, terminalWidth }) {
  return /* @__PURE__ */ jsx(
    Box,
    {
      flexDirection: "column",
      borderStyle: "round",
      borderColor: isLoading ? COLORS.muted : COLORS.primary,
      paddingX: 1,
      marginX: 1,
      marginBottom: 0,
      children: /* @__PURE__ */ jsxs(Box, { flexDirection: "row", gap: 1, children: [
        /* @__PURE__ */ jsx(Text, { color: isLoading ? COLORS.muted : COLORS.primary, bold: true, children: isLoading ? "\u2298" : "\u203A" }),
        isLoading ? /* @__PURE__ */ jsx(Spinner, { label: "Generating response" }) : /* @__PURE__ */ jsx(
          TextInput,
          {
            value,
            onChange,
            onSubmit,
            placeholder: "Type your message... (Enter to send)"
          }
        )
      ] })
    }
  );
}
function WelcomeMessage() {
  return /* @__PURE__ */ jsxs(
    Box,
    {
      flexDirection: "column",
      marginX: 2,
      marginY: 1,
      borderStyle: "round",
      borderColor: COLORS.muted,
      paddingX: 2,
      paddingY: 1,
      children: [
        /* @__PURE__ */ jsx(Text, { color: COLORS.primary, bold: true, children: "Welcome to TUI Chat!" }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: " " }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: "Tips:" }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: "  \u2022 Ask anything \u2014 code, explain, debug" }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: "  \u2022 Press Enter to send a message" }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: "  \u2022 Press Ctrl+C to exit" }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.muted, children: " " }),
        /* @__PURE__ */ jsx(Text, { color: COLORS.accent, children: "Ready. How can I help you today?" })
      ]
    }
  );
}
function App() {
  const { exit } = useApp();
  const { stdout } = useStdout();
  const { isRawModeSupported } = useStdin();
  const [messages, setMessages] = useState([]);
  const [input, setInput] = useState("");
  const [isLoading, setIsLoading] = useState(false);
  const terminalWidth = stdout?.columns ?? 80;
  useInput((_, key) => {
    if (key.ctrl && key.name === "c") {
      exit();
    }
  }, { isActive: isRawModeSupported });
  
  const handleSubmit = useCallback(async (value) => {
    const trimmed = value.trim();
    if (!trimmed || isLoading) return;
    const now = (/* @__PURE__ */ new Date()).toLocaleTimeString("ja-JP", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    });
    const userMsg = {
      id: Date.now(),
      role: "user",
      content: trimmed,
      timestamp: now
    };
    setMessages((prev) => [...prev, userMsg]);
    setInput("");
    setIsLoading(true);
    const delay = 300 + Math.random() * 900;
    await new Promise((r) => setTimeout(r, delay));

    const lib = koffi.load('./libsample.so');
    const rag_search = lib.func('char* rag_search(const char* input)');
    let target = trimmed;
    console.log("target=", target)
    let responseText = "";
    let resp =rag_search(target)
    //win改行文字、変換
    resp = resp.replace(/\r\n/g, "\n");
    responseText = resp;
    const aiNow = (/* @__PURE__ */ new Date()).toLocaleTimeString("ja-JP", {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    });
    const aiMsg = {
      id: Date.now() + 1,
      role: "assistant",
      content: responseText,
      timestamp: aiNow
    };
    setMessages((prev) => [...prev, aiMsg]);
    setIsLoading(false);
  }, [isLoading]);
  const showWelcome = messages.length === 0 && !isLoading;
  return /* @__PURE__ */ jsxs(
    Box,
    {
      flexDirection: "column",
      width: terminalWidth,
      minHeight: 24,
      children: [
        /* @__PURE__ */ jsx(Header, { terminalWidth }),
        /* @__PURE__ */ jsxs(Box, { flexDirection: "column", flexGrow: 1, paddingX: 1, children: [
          showWelcome && /* @__PURE__ */ jsx(WelcomeMessage, {}),
          messages.map((msg) => /* @__PURE__ */ jsx(
            MessageBubble,
            {
              message: msg,
              terminalWidth
            },
            msg.id
          )),
          isLoading && /* @__PURE__ */ jsx(Box, { marginLeft: 3, marginY: 0, children: /* @__PURE__ */ jsx(Spinner, { label: `${AI_NAME} is thinking` }) })
        ] }),
        /* @__PURE__ */ jsx(
          StatusBar,
          {
            messageCount: messages.length,
            isLoading,
            terminalWidth
          }
        ),
        /* @__PURE__ */ jsx(
          InputArea,
          {
            value: input,
            onChange: setInput,
            onSubmit: handleSubmit,
            isLoading,
            terminalWidth
          }
        )
      ]
    }
  );
}
render(/* @__PURE__ */ jsx(App, {}));
