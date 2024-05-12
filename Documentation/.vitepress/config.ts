import { defineConfig } from "vitepress"
import { createRequire } from "node:module"

const require = createRequire(import.meta.url)

export default defineConfig({
    title: "TonyTools",
    description: "Documentation hub for the TonyTools suite of libraries and tools.",
    srcExclude: ["**/README.md"],
    themeConfig: {
        logo: "/images/logo.png",
        nav: [
            { text: "Glacier Modding", link: "https://glaciermodding.org" },
            { text: "Hitman Modding Resources", link: "https://hitman-resources.netlify.app" }
        ],
        sidebar: [
            {
                text: "General",
                items: [
                    { text: "Installation", link: "/general/installation" },
                    { text: "Third-Party", link: "/general/thirdparty" }
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
                    { text: "GFXFzip", link: "/tools/gfxfzip" },
                    { text: "HMTextureTools", link: "/tools/hmtexturetools" },
                    { text: "HMLanguageTools", link: "/tools/hmlanguagetools" },
                    { text: "HMAScrambler", link: "/tools/hmascrambler" }
                ]
            },
            {
                text: "Dev Tools",
                collapsed: true,
                items: [
                    { text: "BOXCExporter", link: "/tools/boxcexporter" },
                    { text: "JSONPatchCreator", link: "/tools/jsonpatchcreator" },
                    { text: "MATE", link: "/tools/mate" },
                    { text: "MJBATesting", link: "/tools/mjbatesting" },
                    { text: "SCDA", link: "/tools/scda" },
                    { text: "VTXD", link: "/tools/vtxd" }
                ]
            }
        ],
        socialLinks: [
            { icon: "github", link: "https://github.com/AnthonyFuller/TonyTools" }
        ],
        search: {
            provider: "algolia",
            options: {
                appId: "QP9T39ARJM",
                apiKey: "4cb3a30158f05ef58bd70da03683dbc2",
                indexName: "tonytools"
            }
        }
    },
    cleanUrls: true,
    vite: {
        resolve: {
            alias: {
                "vue/server-renderer": require.resolve("vue/server-renderer"),
            }
        }
    },
    sitemap: {
        hostname: "https://tonytools.win"
    }
})
