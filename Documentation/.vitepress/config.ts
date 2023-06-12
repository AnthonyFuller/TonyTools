import { defineConfig } from "vitepress"
import { SitemapStream } from 'sitemap'
import { createWriteStream } from 'node:fs'
import { resolve } from 'node:path'

const links: any = []

export default defineConfig({
    title: "TonyTools",
    description: "Documentation hub for the TonyTools suite of libraries and tools.",
    themeConfig: {
        logo: "/logo.png",
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
    // Sitemap setup
    transformHtml: (_, id, { pageData }) => {
        if (!/[\\/]404\.html$/.test(id))
            links.push({
                url: pageData.relativePath.replace(/((^|\/)index)?\.md$/, '$2'),
                changefreq: pageData.params?.changefreq ?? "weekly",
                priority: pageData.params?.priority ?? 0.5
            })
    },
    buildEnd: async ({ outDir }) => {
        const sitemap = new SitemapStream({
            hostname: 'https://tonytools.win'
        })
        const writeStream = createWriteStream(resolve(outDir, 'sitemap.xml'))
        sitemap.pipe(writeStream)
        links.forEach((link) => sitemap.write(link))
        sitemap.end()
        await new Promise((r) => writeStream.on('finish', r))
    }
})
