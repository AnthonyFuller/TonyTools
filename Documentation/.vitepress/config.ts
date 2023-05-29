import { defineConfig } from "vitepress"

export default defineConfig({
    title: "TonyTools",
    themeConfig: {
        sidebar: [
            {
                text: "General",
                items: [
                    { text: "Installation", link: "/general/installation" }
                ]
            },
            {
                text: "Libraries",
                items: [
                    { text: "HMLanguages", link: "/libraries/hmlanguages" }
                ]
            },
            {
                text: "Tools",
                items: [
                    { text: "HMTextureTools", link: "/tools/hmtexturetools" },
                    { text: "HMLanguageTools", link: "/tools/hmlanguagetools" },
                    { text: "HMAScrambler", link: "/tools/hmascrambler" },
                    { text: "BOXCExporter", link: "/tools/boxcexporter" },
                    { text: "JSONPatchCreator", link: "/tools/jsonpatchcreator" },
                    { text: "MJBATesting", link: "/tools/mjbatesting" },
                    { text: "SCDA", link: "/tools/scda" },
                    { text: "VTXD", link: "/tools/vtxd" }
                ]
            }
        ],
        socialLinks: [
            { icon: "github", link: "https://github.com/AnthonyFuller/TonyTools" }
        ]
    }
})
