GETTEXT_CREATE_TRANSLATIONS(stellarium-skycultures DEFAULT_TARGET
    aa ab ae af ak am an ar as ast av az ba be bg bh bi bn bo br bs ca ce ckb cs cv cy da de dv el en en_AU en_CA en_GB en_US eo es et eu fa fi fil fj fr ga gd gl gn gu haw he hi hr hrx ht hu hy ia id is it ja ka kg kk kn ko ky la lb lo lt lv mi mk ml mn mo mr ms mt nan nb nl nn oc os pa pl pt pt_BR ro ru sah se si sk sl sm sq sr su sv sw ta te tg th tl tr tt uk uz vi zh_CN zh_HK zh_TW zu
)

# Discard this target if xgettext is not installed
IF(${XGETTEXT_FOUND})
	ADD_CUSTOM_TARGET(
		generate-pot-stellarium-skycultures
		${GETTEXT_XGETTEXT_EXECUTABLE}
			-o stellarium-skycultures.pot
			--keyword=_
			-C
			--directory=${PROJECT_SOURCE_DIR}
			--files-from=POTFILES.in
			--copyright-holder=Stellarium's\ team
		--from-code=utf-8
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating stellarium-skycultures.pot"
		VERBATIM
	)
	
	# Generate this POT file when building the "generate-pot" target.
	ADD_DEPENDENCIES(generate-pot generate-pot-stellarium-skycultures)
ENDIF()
