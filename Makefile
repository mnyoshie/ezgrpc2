all:


push:
	git add . && git commit -S && cat ~/kmnyoshie | \
		termux-clipboard-set && git push $(f)
