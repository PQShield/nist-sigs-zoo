<script lang="ts">
	import { onMount } from 'svelte';
	import { base } from '$app/paths';
	import { legacyPathRedirect } from '$lib/legacy';
	import { SITE_NAME } from '$lib/constants';

	interface Props {
		/** Target path relative to base, no leading slash, e.g. "signatures/advanced/". */
		to: string;
		title: string;
	}
	let { to, title }: Props = $props();

	const href = $derived(`${base}/${to}`);

	onMount(() => {
		location.replace(legacyPathRedirect(to, location.search, location.hash, base));
	});
</script>

<svelte:head>
	<title>{title} — {SITE_NAME}</title>
	<link rel="canonical" {href} />
	<!-- No-JS fallback only: the delay lets the JS redirect (which keeps search + hash) win. -->
	<meta http-equiv="refresh" content="3; url={href}" />
	<meta name="robots" content="noindex" />
</svelte:head>

<div class="mx-auto max-w-screen-md px-6 py-10">
	<p class="text-sm">This page has moved to <a class="text-pqs-steel underline dark:text-pqs-apricot" {href}>{title}</a>.</p>
</div>
